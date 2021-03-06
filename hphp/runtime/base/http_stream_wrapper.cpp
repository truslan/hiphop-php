/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-2013 Facebook, Inc. (http://www.facebook.com)     |
   | Copyright (c) 1998-2010 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
*/

#include "hphp/runtime/base/http_stream_wrapper.h"
#include "hphp/runtime/base/string_util.h"
#include "hphp/runtime/base/url_file.h"
#include "hphp/runtime/base/runtime_option.h"
#include "hphp/runtime/ext/ext_stream.h"
#include <memory>

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

const StaticString
  s_GET("GET"),
  s_method("method"),
  s_http("http"),
  s_header("header"),
  s_max_redirects("max_redirects"),
  s_timeout("timeout"),
  s_content("content");

File* HttpStreamWrapper::open(CStrRef filename, CStrRef mode,
                              int options, CVarRef context) {
  if (RuntimeOption::ServerHttpSafeMode) {
    return nullptr;
  }

  if (strncmp(filename.data(), "http://",  sizeof("http://")  - 1) &&
      strncmp(filename.data(), "https://", sizeof("https://") - 1)) {
    return nullptr;
  }

  std::unique_ptr<UrlFile> file;
  StreamContext *ctx = !context.isResource() ? nullptr :
                        context.toResource().getTyped<StreamContext>();
  if (!ctx || ctx->m_options.isNull() || ctx->m_options[s_http].isNull()) {
    file = std::unique_ptr<UrlFile>(NEWOBJ(UrlFile)());
  } else {
    Array opts = ctx->m_options[s_http].toArray();
    String method = s_GET;
    if (opts.exists(s_method)) {
      method = opts[s_method].toString();
    }
    Array headers;
    if (opts.exists(s_header)) {
      Array lines = StringUtil::Explode(
        opts[s_header].toString(), "\r\n").toArray();
      for (ArrayIter it(lines); it; ++it) {
        Array parts = StringUtil::Explode(
          it.second().toString(), ": ").toArray();
        headers.set(parts.rvalAt(0), parts.rvalAt(1));
      }
    }
    static const StaticString s_user_agent("user_agent");
    static const StaticString s_User_Agent("User-Agent");
    if (opts.exists(s_user_agent) && !headers.exists(s_User_Agent)) {
      headers.set(s_User_Agent, opts[s_user_agent]);
    }
    int max_redirs = 20;
    if (opts.exists(s_max_redirects)) {
      max_redirs = opts[s_max_redirects].toInt64();
    }
    int timeout = -1;
    if (opts.exists(s_timeout)) {
      timeout = opts[s_timeout].toInt64();
    }
    file = std::unique_ptr<UrlFile>(NEWOBJ(UrlFile)(method.data(), headers,
                                                    opts[s_content].toString(),
                                                    max_redirs, timeout));
  }
  bool ret = file->open(filename, mode);
  if (!ret) {
    raise_warning("Failed to open %s (%s)", filename.data(),
                  file->getLastError().c_str());
    return nullptr;
  }
  return file.release();
}

///////////////////////////////////////////////////////////////////////////////
}
