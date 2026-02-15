class STDINOUT2
{
public:

	bool Offx = false;
	HANDLE g_hChildStd_IN_Rd = NULL;
	HANDLE g_hChildStd_IN_Wr = NULL;
	HANDLE g_hChildStd_OUT_Rd = NULL;
	HANDLE g_hChildStd_OUT_Wr = NULL;
	HANDLE g_hChildStd_ERR_Rd = NULL;
	HANDLE g_hChildStd_ERR_Wr = NULL;
	PROCESS_INFORMATION piProcInfo = {};
	STARTUPINFO siStartInfo = {};

	void Off()
	{
		Offx = true;
		CloseHandle(g_hChildStd_IN_Rd);
		g_hChildStd_IN_Rd = 0;
		CloseHandle(g_hChildStd_IN_Wr);
		g_hChildStd_IN_Wr = 0;

		if (g_hChildStd_ERR_Wr)
			CloseHandle(g_hChildStd_ERR_Wr);
		g_hChildStd_ERR_Wr = 0;
		if (g_hChildStd_ERR_Rd)
			CloseHandle(g_hChildStd_ERR_Rd);
		g_hChildStd_ERR_Rd = 0;

		CloseHandle(g_hChildStd_OUT_Wr);
		g_hChildStd_OUT_Wr = 0;
		CloseHandle(g_hChildStd_OUT_Rd);
		g_hChildStd_OUT_Rd = 0;
		WaitForSingleObject(piProcInfo.hProcess, INFINITE);
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
		piProcInfo.hProcess = 0;
		piProcInfo.hThread = 0;
	}

	bool Prep(bool StdErr = false)
	{
		SECURITY_ATTRIBUTES saAttr = {};
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;

		Offx = false;
		if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
			return false;

	  // Ensure the read handle to the pipe for STDOUT is not inherited.
		if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
			return false;

		  // Create a pipe for the child process's STDIN. 
		if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
			return false;

	  // Ensure the write handle to the pipe for STDIN is not inherited. 
		if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
			return false;

		if (StdErr)
		{
			if (!CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &saAttr, 0))
				return false;

		  // Ensure the read handle to the pipe for STDERR is not inherited.
			if (!SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0))
				return false;
		}

		return true;
	}

	bool CreateChildProcess(const wchar_t* cd,wchar_t* szCmdline,bool Show)
	{
		BOOL bSuccess = FALSE;

		 // Set up members of the PROCESS_INFORMATION structure. 

		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.hStdError = g_hChildStd_OUT_Wr;
		siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
		if (g_hChildStd_ERR_Wr)
			siStartInfo.hStdError = g_hChildStd_ERR_Wr;
		siStartInfo.hStdInput = g_hChildStd_IN_Rd;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	 // Create the child process. 

		bSuccess = CreateProcess(NULL,
			szCmdline,     // command line 
			NULL,          // process security attributes 
			NULL,          // primary thread security attributes 
			TRUE,          // handles are inherited 
			Show ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW,             // creation flags 
			NULL,          // use parent's environment 
			cd,          // use parent's current directory 
			&siStartInfo,  // STARTUPINFO pointer 
			&piProcInfo);  // receives PROCESS_INFORMATION 

		 // If an error occurs, exit the application. 
		if (!bSuccess)
			return false;

		SetPriorityClass(piProcInfo.hProcess, IDLE_PRIORITY_CLASS);
		SetThreadPriority(piProcInfo.hThread, THREAD_PRIORITY_IDLE);
		return true;
	}

	bool WriteToPipe(const char* chBuf, DWORD dwRead)
	{
		DWORD dwWritten;
		BOOL bSuccess = FALSE;
		DWORD ec = 0;
		GetExitCodeProcess(piProcInfo.hProcess, &ec);
		if (ec != STILL_ACTIVE)
			return 0;
		bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, dwRead, &dwWritten, NULL);
		return bSuccess;
	}

	void Read()
	{
#ifndef _DEBUG
		return;
#endif
		for (;;)
		{
			if (Offx)
				break;
			std::vector<char> d(10000);
			DWORD r;
			ReadFromPipe(d.data(), 10000, &r);
			const char* a21 = d.data();
			OutputDebugStringA(a21);
		}
	}
	bool ReadFromPipe(char* chBuf, DWORD sz, DWORD* dwRead,bool Err = false)

	// Read output from the child process's pipe for STDOUT
	// and write to the parent process's pipe for STDOUT. 
	// Stop when there is no more data. 
	{
		BOOL bSuccess = FALSE;
		try
		{
			if (Offx)
				return 0;
			bSuccess = ReadFile(Err ? g_hChildStd_ERR_Rd : g_hChildStd_OUT_Rd, chBuf, sz, dwRead, NULL);

		}
		catch (...)
		{
		}
		return bSuccess;
	}

};

