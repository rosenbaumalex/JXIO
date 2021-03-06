/*
** Copyright (C) 2013 Mellanox Technologies
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at:
**
** http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
** either express or implied. See the License for the specific language
** governing permissions and  limitations under the License.
**
*/
#include "Utils.h"
#include "Bridge.h"
#include <libxio.h>

log_severity_t g_log_threshold = DEFAULT_LOG_THRESHOLD;
log_severity_t g_xio_log_level_to_jxio_severity[XIO_LOG_LEVEL_LAST];

void log_set_threshold(log_severity_t _threshold)
{
	g_log_threshold = (lsNONE <= _threshold && _threshold <= lsTRACE) ? _threshold : DEFAULT_LOG_THRESHOLD;
}

void log_func(log_severity_t severity, const char *log_fmt, ...)
{
	const int SIZE = 2048;
	char _str_[SIZE];
	va_list ap;
	va_start(ap, log_fmt);
	int m = vsnprintf(_str_, SIZE, log_fmt, ap);
	va_end(ap);
	if (m < 0) {
		return; /*error*/
	}
	_str_[SIZE-1] = '\0';
	Bridge_invoke_logToJava_callback(severity, _str_);
}

void logs_from_xio_callback(const char *file, unsigned line, const char *func, unsigned level, const char *log_fmt, ...)
{
	log_severity_t severity = g_xio_log_level_to_jxio_severity[level];
	if (severity > g_log_threshold)
		return;

	const int SIZE = 2048;
	char _str_[SIZE];
	int n = snprintf(_str_, SIZE, MODULE_FILE_INFO, file, line, func);
	if (n < 0) {
		return; /*error*/
	}
	if (n < SIZE) {
		va_list ap;
		va_start(ap, log_fmt);
		int m = vsnprintf(_str_ + n, SIZE - n, log_fmt, ap);
		va_end(ap);
		if (m < 0) {
			return; /*error*/
		}
	}
	_str_[SIZE - 1] = '\0';
	Bridge_invoke_logToJava_callback(severity, _str_);
}

void logs_from_xio_callback_register()
{
	// init log level/severity conversion table
	g_xio_log_level_to_jxio_severity[XIO_LOG_LEVEL_FATAL] = lsFATAL;
	g_xio_log_level_to_jxio_severity[XIO_LOG_LEVEL_ERROR] = lsERROR;
	g_xio_log_level_to_jxio_severity[XIO_LOG_LEVEL_WARN] = lsWARN;
	g_xio_log_level_to_jxio_severity[XIO_LOG_LEVEL_INFO] = lsINFO;
	g_xio_log_level_to_jxio_severity[XIO_LOG_LEVEL_DEBUG] = lsDEBUG;
	g_xio_log_level_to_jxio_severity[XIO_LOG_LEVEL_TRACE] = lsTRACE;

	int optlen = sizeof(xio_log_fn);
	const void* optval = (const void *)logs_from_xio_callback;
	xio_set_opt(NULL, XIO_OPTLEVEL_ACCELIO, XIO_OPTNAME_LOG_FN, optval, optlen);
}

void logs_from_xio_callback_unregister()
{
	xio_set_opt(NULL, XIO_OPTLEVEL_ACCELIO, XIO_OPTNAME_LOG_FN, NULL, 0);
}

void logs_from_xio_set_threshold(log_severity_t threshold)
{
	int xio_log_level = XIO_LOG_LEVEL_INFO;

	switch (threshold) {
	case lsFATAL:	xio_log_level = XIO_LOG_LEVEL_FATAL; break;
	case lsERROR:	xio_log_level = XIO_LOG_LEVEL_ERROR; break;
	case lsWARN:	xio_log_level = XIO_LOG_LEVEL_WARN; break;
	case lsDEBUG:	xio_log_level = XIO_LOG_LEVEL_DEBUG; break;
	case lsTRACE:	xio_log_level = XIO_LOG_LEVEL_TRACE; break;

	case lsINFO:
	default:
		xio_log_level = XIO_LOG_LEVEL_INFO; break;
	}
	xio_set_opt(NULL, XIO_OPTLEVEL_ACCELIO, XIO_OPTNAME_LOG_LEVEL, &xio_log_level, sizeof(enum xio_log_level));
}

ServerSession* get_ses_server_for_session(xio_session* session, bool to_delete)
{
	map_ses_ctx_t::iterator it;
	pthread_mutex_lock(&mutex_for_map);
	it=map_sessions.find(session);
	if (it == map_sessions.end()){
		LOG_ERR("session=%p not in map", session);
		pthread_mutex_unlock(&mutex_for_map);
		return NULL;
	}
	ServerSession *jxio_session = it->second;
	if (to_delete){
		map_sessions.erase(it);
	}
	pthread_mutex_unlock(&mutex_for_map);
	if (to_delete)
		LOG_DBG("deleting pair <jxio_session=%p, xio_session=%p>", jxio_session, session);
	else
		LOG_DBG("returning pair <jxio_session=%p, xio_session=%p>", jxio_session, session);

	return jxio_session;
}

void add_ses_server_for_session(xio_session * xio_session, ServerSession* jxio_session)
{
	LOG_DBG("adding pair<jxio_session=%p, xio_session=%p>", jxio_session, xio_session);
	pthread_mutex_lock(&mutex_for_map);
	map_sessions.insert(pair_ses_ctx_t(xio_session, jxio_session));
	pthread_mutex_unlock(&mutex_for_map);
}

bool close_xio_connection(struct xio_session *session, struct xio_context *ctx)
{
	LOG_DBG("closing connection for session=%p, context=%p", session, ctx);
	xio_connection * con = xio_get_connection(session, ctx);
	if (con == NULL) {
		LOG_DBG("ERROR, no connection found (xio_session=%p, xio_context=%p)", session, ctx);
		return false;
	}
	if (xio_disconnect(con)) {
		LOG_DBG("ERROR, xio_disconnect failed with error '%s' (%d) (xio_session=%p, xio_context=%p, conn=%p)",
				xio_strerror(xio_errno()), xio_errno(), session, ctx, con);
		return false;
	}
	LOG_DBG("successfully closed connection=%p, for session=%p, context=%p", con, session, ctx);
	return true;
}

bool forward_session(ServerSession* jxio_session, const char * url)
{
	struct xio_session *xio_session = jxio_session->get_xio_session();
	LOG_DBG("url before forward is %s. xio_session is %p", url, xio_session);

	int retVal = xio_accept(xio_session, &url, 1, NULL, 0);
	if (retVal) {
		LOG_DBG("ERROR, accepting session=%p. error '%s' (%d)", xio_session, xio_strerror(xio_errno()), xio_errno());
		return false;
	}
	add_ses_server_for_session(xio_session, jxio_session);
	jxio_session->set_ignore_first_disconnect();
	return true;
}

bool accept_session(ServerSession* jxio_session)
{
	struct xio_session *xio_session = jxio_session->get_xio_session();
	LOG_DBG("before accept xio_session is %p", xio_session);

	int retVal = xio_accept(xio_session, NULL, 0, NULL, 0);
	if (retVal) {
		LOG_DBG("ERROR, accepting session=%p. error '%s' (%d)",xio_session, xio_strerror(xio_errno()), xio_errno());
		return false;
	}
	add_ses_server_for_session(xio_session, jxio_session);
	return true;
}


bool reject_session(ServerSession* jxio_session, int reason,
		char *user_context, size_t user_context_len)
{
	struct xio_session *xio_session = jxio_session->get_xio_session();
	LOG_DBG("before reject xio_session=%p. reason is %d", xio_session, reason);

	enum xio_status s = (enum xio_status)(reason + XIO_BASE_STATUS -1);

	int retVal = xio_reject(xio_session, s, user_context, user_context_len);
	if (retVal) {
		LOG_DBG("ERROR, rejecting session=%p. error '%s' (%d)",xio_session, xio_strerror(xio_errno()), xio_errno());
		return false;
	}
	add_ses_server_for_session(xio_session, jxio_session);
	jxio_session->delete_after_teardown = true;
	return true;
}
