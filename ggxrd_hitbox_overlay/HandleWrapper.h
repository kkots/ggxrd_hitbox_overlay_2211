#pragma once

struct HandleWrapper {
	HANDLE value = NULL;
	HandleWrapper() = default;
	HandleWrapper(HANDLE source);
	HandleWrapper& operator=(HANDLE source);
	operator HANDLE ();
	~HandleWrapper();
	HandleWrapper& operator=(const HandleWrapper& source) = delete;
	HandleWrapper(const HandleWrapper& source) = delete;
	HandleWrapper& operator=(HandleWrapper&& source) noexcept;
	HandleWrapper(HandleWrapper&& source) noexcept;
	void move(HandleWrapper& source);
	void close();
};
