#include "../includes/webserv.hpp"

const std::map<StatusCode, std::string> STATUS_MESSAGES;
const std::map<std::string, std::string> MIME_TYPES;

static void setStatusMessage(StatusCode statusCode, const std::string& message) {
	const_cast<std::map<StatusCode, std::string>&>(STATUS_MESSAGES)[statusCode] = message;
}

void initStatusMessageMap() {
	setStatusMessage(INFORMATIONAL_CONTINUE, "Continue");
	setStatusMessage(INFORMATIONAL_SWITCHING_PROTOCOLS, "Switching Protocols");
	setStatusMessage(INFORMATIONAL_PROCESSING, "Processing");
	setStatusMessage(INFORMATIONAL_EARLY_HINTS, "Early Hints");
	setStatusMessage(SUCCESS_OK, "OK");
	setStatusMessage(SUCCESS_CREATED, "Created");
	setStatusMessage(SUCCESS_ACCEPTED, "Accepted");
	setStatusMessage(SUCCESS_NON_AUTHORITATIVE_INFORMATION, "Non-Authoritative Information");
	setStatusMessage(SUCCESS_NO_CONTENT, "No Content");
	setStatusMessage(SUCCESS_RESET_CONTENT, "Reset Content");
	setStatusMessage(SUCCESS_PARTIAL_CONTENT, "Partial Content");
	setStatusMessage(SUCCESS_MULTI_STATUS, "Multi-Status");
	setStatusMessage(SUCCESS_ALREADY_REPORTED, "Already Reported");
	setStatusMessage(SUCCESS_IM_USED, "IM Used");
	setStatusMessage(REDIRECTION_MULTIPLE_CHOICES, "Multiple Choices");
	setStatusMessage(REDIRECTION_MOVED_PERMANENTLY, "Moved Permanently");
	setStatusMessage(REDIRECTION_FOUND, "Found");
	setStatusMessage(REDIRECTION_SEE_OTHER, "See Other");
	setStatusMessage(REDIRECTION_NOT_MODIFIED, "Not Modified");
	setStatusMessage(REDIRECTION_USE_PROXY, "Use Proxy");
	setStatusMessage(REDIRECTION_TEMPORARY_REDIRECT, "Temporary Redirect");
	setStatusMessage(REDIRECTION_PERMANENT_REDIRECT, "Permanent Redirect");
	setStatusMessage(CLIENT_BAD_REQUEST, "Bad Request");
	setStatusMessage(CLIENT_UNAUTHORIZED, "Unauthorized");
	setStatusMessage(CLIENT_PAYMENT_REQUIRED, "Payment Required");
	setStatusMessage(CLIENT_FORBIDDEN, "Forbidden");
	setStatusMessage(CLIENT_NOT_FOUND, "Not Found");
	setStatusMessage(CLIENT_METHOD_NOT_ALLOWED, "Method Not Allowed");
	setStatusMessage(CLIENT_NOT_ACCEPTABLE, "Not Acceptable");
	setStatusMessage(CLIENT_PROXY_AUTHENTICATION_REQUIRED, "Proxy Authentication Required");
	setStatusMessage(CLIENT_REQUEST_TIMEOUT, "Request Timeout");
	setStatusMessage(CLIENT_CONFLICT, "Conflict");
	setStatusMessage(CLIENT_GONE, "Gone");
	setStatusMessage(CLIENT_LENGTH_REQUIRED, "Length Required");
	setStatusMessage(CLIENT_PRECONDITION_FAILED, "Precondition Failed");
	setStatusMessage(CLIENT_PAYLOAD_TOO_LARGE, "Payload Too Large");
	setStatusMessage(CLIENT_URI_TOO_LONG, "URI Too Long");
	setStatusMessage(CLIENT_UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type");
	setStatusMessage(CLIENT_RANGE_NOT_SATISFIABLE, "Range Not Satisfiable");
	setStatusMessage(CLIENT_EXPECTATION_FAILED, "Expectation Failed");
	setStatusMessage(CLIENT_IM_A_TEAPOT, "I'm a teapot");
	setStatusMessage(CLIENT_MISDIRECTED_REQUEST, "Misdirected Request");
	setStatusMessage(CLIENT_UNPROCESSABLE_ENTITY, "Unprocessable Entity");
	setStatusMessage(CLIENT_LOCKED, "Locked");
	setStatusMessage(CLIENT_FAILED_DEPENDENCY, "Failed Dependency");
	setStatusMessage(CLIENT_TOO_EARLY, "Too Early");
	setStatusMessage(CLIENT_UPGRADE_REQUIRED, "Upgrade Required");
	setStatusMessage(CLIENT_PRECONDITION_REQUIRED, "Precondition Required");
	setStatusMessage(CLIENT_TOO_MANY_REQUESTS, "Too Many Requests");
	setStatusMessage(CLIENT_REQUEST_HEADER_FIELDS_TOO_LARGE, "Request Header Fields Too Large");
	setStatusMessage(CLIENT_UNAVAILABLE_FOR_LEGAL_REASONS, "Unavailable For Legal Reasons");
	setStatusMessage(SERVER_INTERNAL_SERVER_ERROR, "Internal Server Error");
	setStatusMessage(SERVER_NOT_IMPLEMENTED, "Not Implemented");
	setStatusMessage(SERVER_BAD_GATEWAY, "Bad Gateway");
	setStatusMessage(SERVER_SERVICE_UNAVAILABLE, "Service Unavailable");
	setStatusMessage(SERVER_GATEWAY_TIMEOUT, "Gateway Timeout");
	setStatusMessage(SERVER_HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported");
	setStatusMessage(SERVER_VARIANT_ALSO_NEGOTIATES, "Variant Also Negotiates");
	setStatusMessage(SERVER_INSUFFICIENT_STORAGE, "Insufficient Storage");
	setStatusMessage(SERVER_LOOP_DETECTED, "Loop Detected");
	setStatusMessage(SERVER_NOT_EXTENDED, "Not Extended");
	setStatusMessage(SERVER_NETWORK_AUTHENTICATION_REQUIRED, "Network Authentication Required");
}

static void setMimeType(const std::string& extension, const std::string& mime) {
	const_cast<std::map<std::string, std::string>&>(MIME_TYPES)[extension] = mime;
}

void initMimeTypes() {
	setMimeType("css", "text/css");
	setMimeType("html", "text/html");
	setMimeType("jpg", "image/jpeg");
	setMimeType("js", "text/javascript");
	setMimeType("mp4", "video/mp4");
	setMimeType("png", "image/png");
	setMimeType("svg", "image/svg+xml");

	std::ifstream ifs("/etc/mime.types");
	std::string line, mime, extension;
	while (true) {
		if (!std::getline(ifs, line))
			return;
		std::stringstream lineStream(line);
		if (line.empty() || line[0] == '#' || !(lineStream >> mime))
			continue;
		while (lineStream >> extension)
			setMimeType(extension, mime);
	}
}