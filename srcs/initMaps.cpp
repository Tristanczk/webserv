#include "../includes/webserv.hpp"

const std::map<StatusCode, std::string> STATUS_MESSAGES;
const std::map<std::string, std::string> MIME_TYPES;

static void setStatusMessage(StatusCode statusCode, const std::string& message) {
	const_cast<std::map<StatusCode, std::string>&>(STATUS_MESSAGES)[statusCode] = message;
}

void initStatusMessageMap() {
	setStatusMessage(STATUS_CONTINUE, "Continue");
	setStatusMessage(STATUS_SWITCHING_PROTOCOLS, "Switching Protocols");
	setStatusMessage(STATUS_PROCESSING, "Processing");
	setStatusMessage(STATUS_EARLY_HINTS, "Early Hints");
	setStatusMessage(STATUS_OK, "OK");
	setStatusMessage(STATUS_CREATED, "Created");
	setStatusMessage(STATUS_ACCEPTED, "Accepted");
	setStatusMessage(STATUS_NON_AUTHORITATIVE_INFORMATION, "Non-Authoritative Information");
	setStatusMessage(STATUS_NO_CONTENT, "No Content");
	setStatusMessage(STATUS_RESET_CONTENT, "Reset Content");
	setStatusMessage(STATUS_PARTIAL_CONTENT, "Partial Content");
	setStatusMessage(STATUS_MULTI_STATUS, "Multi-Status");
	setStatusMessage(STATUS_ALREADY_REPORTED, "Already Reported");
	setStatusMessage(STATUS_IM_USED, "IM Used");
	setStatusMessage(STATUS_MULTIPLE_CHOICES, "Multiple Choices");
	setStatusMessage(STATUS_MOVED_PERMANENTLY, "Moved Permanently");
	setStatusMessage(STATUS_FOUND, "Found");
	setStatusMessage(STATUS_SEE_OTHER, "See Other");
	setStatusMessage(STATUS_NOT_MODIFIED, "Not Modified");
	setStatusMessage(STATUS_USE_PROXY, "Use Proxy");
	setStatusMessage(STATUS_TEMPORARY_REDIRECT, "Temporary Redirect");
	setStatusMessage(STATUS_PERMANENT_REDIRECT, "Permanent Redirect");
	setStatusMessage(STATUS_BAD_REQUEST, "Bad Request");
	setStatusMessage(STATUS_UNAUTHORIZED, "Unauthorized");
	setStatusMessage(STATUS_PAYMENT_REQUIRED, "Payment Required");
	setStatusMessage(STATUS_FORBIDDEN, "Forbidden");
	setStatusMessage(STATUS_NOT_FOUND, "Not Found");
	setStatusMessage(STATUS_METHOD_NOT_ALLOWED, "Method Not Allowed");
	setStatusMessage(STATUS_NOT_ACCEPTABLE, "Not Acceptable");
	setStatusMessage(STATUS_PROXY_AUTHENTICATION_REQUIRED, "Proxy Authentication Required");
	setStatusMessage(STATUS_REQUEST_TIMEOUT, "Request Timeout");
	setStatusMessage(STATUS_CONFLICT, "Conflict");
	setStatusMessage(STATUS_GONE, "Gone");
	setStatusMessage(STATUS_LENGTH_REQUIRED, "Length Required");
	setStatusMessage(STATUS_PRECONDITION_FAILED, "Precondition Failed");
	setStatusMessage(STATUS_PAYLOAD_TOO_LARGE, "Payload Too Large");
	setStatusMessage(STATUS_URI_TOO_LONG, "URI Too Long");
	setStatusMessage(STATUS_UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type");
	setStatusMessage(STATUS_RANGE_NOT_SATISFIABLE, "Range Not Satisfiable");
	setStatusMessage(STATUS_EXPECTATION_FAILED, "Expectation Failed");
	setStatusMessage(STATUS_IM_A_TEAPOT, "I'm a teapot");
	setStatusMessage(STATUS_MISDIRECTED_REQUEST, "Misdirected Request");
	setStatusMessage(STATUS_UNPROCESSABLE_ENTITY, "Unprocessable Entity");
	setStatusMessage(STATUS_LOCKED, "Locked");
	setStatusMessage(STATUS_FAILED_DEPENDENCY, "Failed Dependency");
	setStatusMessage(STATUS_TOO_EARLY, "Too Early");
	setStatusMessage(STATUS_UPGRADE_REQUIRED, "Upgrade Required");
	setStatusMessage(STATUS_PRECONDITION_REQUIRED, "Precondition Required");
	setStatusMessage(STATUS_TOO_MANY_REQUESTS, "Too Many Requests");
	setStatusMessage(STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE, "Request Header Fields Too Large");
	setStatusMessage(STATUS_UNAVAILABLE_FOR_LEGAL_REASONS, "Unavailable For Legal Reasons");
	setStatusMessage(STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error");
	setStatusMessage(STATUS_NOT_IMPLEMENTED, "Not Implemented");
	setStatusMessage(STATUS_BAD_GATEWAY, "Bad Gateway");
	setStatusMessage(STATUS_SERVICE_UNAVAILABLE, "Service Unavailable");
	setStatusMessage(STATUS_GATEWAY_TIMEOUT, "Gateway Timeout");
	setStatusMessage(STATUS_HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported");
	setStatusMessage(STATUS_VARIANT_ALSO_NEGOTIATES, "Variant Also Negotiates");
	setStatusMessage(STATUS_INSUFFICIENT_STORAGE, "Insufficient Storage");
	setStatusMessage(STATUS_LOOP_DETECTED, "Loop Detected");
	setStatusMessage(STATUS_NOT_EXTENDED, "Not Extended");
	setStatusMessage(STATUS_NETWORK_AUTHENTICATION_REQUIRED, "Network Authentication Required");
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