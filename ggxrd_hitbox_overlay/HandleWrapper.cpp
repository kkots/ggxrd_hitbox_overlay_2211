#include "pch.h"
#include "HandleWrapper.h"

HandleWrapper::HandleWrapper(HANDLE source) : value(source) { }

HandleWrapper& HandleWrapper::operator=(HANDLE source) {
	close();
	value = source;
	return *this;
}

HandleWrapper::operator HANDLE () {
	return value;
}

HandleWrapper::~HandleWrapper() {
	close();
}

void HandleWrapper::close() {
	if (value) {
		CloseHandle(value);
		value = NULL;
	}
}

HandleWrapper& HandleWrapper::operator=(HandleWrapper&& source) noexcept {
	move(source);
	return *this;
}

HandleWrapper::HandleWrapper(HandleWrapper&& source) noexcept {
	move(source);
}

void HandleWrapper::move(HandleWrapper& source) {
	close();
	value = source.value;
	source.value = NULL;
}
