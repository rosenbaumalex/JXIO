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
#include "Client.h"

#define MODULE_NAME		"Client"
#define CLIENT_LOG_ERR(log_fmt, log_args...)	LOG_BY_MODULE(lsERROR, log_fmt, ##log_args)
#define CLIENT_LOG_WARN(log_fmt, log_args...)	LOG_BY_MODULE(lsWARN, log_fmt, ##log_args)
#define CLIENT_LOG_DBG(log_fmt, log_args...)	LOG_BY_MODULE(lsDEBUG, log_fmt, ##log_args)
#define CLIENT_LOG_TRACE(log_fmt, log_args...)	LOG_BY_MODULE(lsTRACE, log_fmt, ##log_args)


Client::Client(const char* url, long ptrCtx)
{
	CLIENT_LOG_DBG("CTOR start. create client to connect to %s", url);

	struct xio_session_ops ses_ops;
	struct xio_session_attr attr;
	this->error_creating = false;
	this->is_closing = false;

	struct xio_msg *req;

	Context *ctxClass = (Context *) ptrCtx;
	set_ctx_class(ctxClass);

	//defining structs to send to xio library
	ses_ops.on_session_event = on_session_event_callback;
	ses_ops.on_session_established = on_session_established_callback;
	ses_ops.on_msg = on_msg_callback;
	ses_ops.on_msg_error = on_msg_error_callback;

	attr.ses_ops = &ses_ops; /* callbacks structure */
	attr.user_context = NULL; /* no need to pass the server private data */
	attr.user_context_len = 0;

	this->session = xio_session_create(XIO_SESSION_REQ, &attr, url, 0, 0, this);

	if (session == NULL) {
		CLIENT_LOG_ERR("Error in creating session for Context=%p", ctxClass);
		error_creating = true;
		return;
	}

	/* connect the session  */
	this->con = xio_connect(session, ctxClass->ctx, 0, NULL, this);

	if (con == NULL) {
		CLIENT_LOG_ERR("Error in creating connection for Context=%p", ctxClass);
		goto cleanupSes;

	}

	CLIENT_LOG_DBG("CTOR done");
	return;

cleanupSes:
	xio_session_destroy(this->session);
	error_creating = true;
	return;
}

Client::~Client()
{
	CLIENT_LOG_DBG("DTOR done");
}

bool Client::close_connection()
{
	if (this->is_closing){
		CLIENT_LOG_DBG("trying to close connection while already closing");
		return true;
	}
	this->is_closing = true;
	if (xio_disconnect(this->con)) {
		CLIENT_LOG_ERR("Error xio_disconnect failure: '%s' (%d)", xio_strerror(xio_errno()), xio_errno());
		return false;
	}

	CLIENT_LOG_DBG("connection closed successfully");
	return true;
}

Context* Client::ctxForSessionEvent(struct xio_session_event_data * event, struct xio_session *session)
{
	Context *ctx;
	switch (event->event) {
	case XIO_SESSION_CONNECTION_CLOSED_EVENT: //event created because user on this side called "close"
		CLIENT_LOG_DBG("got XIO_SESSION_CONNECTION_CLOSED_EVENT");
		this->is_closing = true;
		return NULL;

	case XIO_SESSION_CONNECTION_TEARDOWN_EVENT:
		CLIENT_LOG_DBG("got XIO_SESSION_CONNECTION_TEARDOWN_EVENT");
		xio_connection_destroy(event->conn);
		return NULL;

	case XIO_SESSION_NEW_CONNECTION_EVENT:
		CLIENT_LOG_DBG("got XIO_SESSION_NEW_CONNECTION_EVENT");
		return NULL;

	case XIO_SESSION_CONNECTION_DISCONNECTED_EVENT: //event created "from underneath"
		CLIENT_LOG_DBG("got XIO_SESSION_CONNECTION_DISCONNECTED_EVENT");
		close_connection();
		return NULL;

	case XIO_SESSION_TEARDOWN_EVENT:
		CLIENT_LOG_DBG("got XIO_SESSION_TEARDOWN_EVENT. must delete session");
		if (!this->is_closing){
			CLIENT_LOG_ERR("Got session teardown without getting connection/close/disconnected/rejected");
		}
		//the event should also be written to buffer to let user know that the session was closed
		if (xio_session_destroy(session)) {
			CLIENT_LOG_ERR("Error xio_session_close failure: '%s' (%d) ", xio_strerror(xio_errno()), xio_errno());
		}
		return this->get_ctx_class();

	case XIO_SESSION_REJECT_EVENT:
		CLIENT_LOG_DBG("got XIO_SESSION_REJECT_EVENT. must delete session");
		this->is_closing = true;
		return this->get_ctx_class();

	case XIO_SESSION_CONNECTION_ERROR_EVENT:
		CLIENT_LOG_DBG("got XIO_SESSION_CONNECTION_ERROR_EVENT");
		close_connection();
		return NULL;

	case XIO_SESSION_ERROR_EVENT:
	default:
		CLIENT_LOG_WARN("UNHANDLED event: got event '%s' (%d)",  xio_session_event_str(event->event), event->event);
		return this->get_ctx_class();
	}
}

bool Client::send_msg(Msg *msg, const int size)
{
	if (this->is_closing) {
		CLIENT_LOG_DBG("attempting to send a message while client session is closing");
		return false;
	}
	CLIENT_LOG_TRACE("##################### sending msg=%p, size=%d", msg, size);
	msg->set_xio_msg_out_size(size);
	msg->reset_xio_msg_in_size();
	int ret_val = xio_send_request(this->con, msg->get_xio_msg());
	if (ret_val) {
		CLIENT_LOG_ERR("Error in sending xio_msg: '%s' (%d)", xio_strerror(xio_errno()), xio_errno());
		return false;
	}
	return true;
}
