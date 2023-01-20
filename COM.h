#pragma once

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h> 
#include <iostream>  
#include <cstring>

#define EX_FATAL 1
//Crea canale di comunicazione con la COM
bool createCOM(HANDLE* handleCOMM, char* COM) {

	*handleCOMM = CreateFile(
		COM,										// Assign the COM port
		GENERIC_READ | GENERIC_WRITE,				// Open the channel with read/write privileges
		NULL,										// No sharing allowed
		NULL,										// No security
		OPEN_EXISTING,								// Opens the existing com port
		FILE_ATTRIBUTE_NORMAL,						// Do not set any file attributes --> Use synchronous operation
		NULL										// No template
	);

	if (*handleCOMM == INVALID_HANDLE_VALUE) {
		std::cout << "COM non puo' essere creata. Errore: " << GetLastError();
		return false;

	}
	else {
		std::cout << "COM e' stata creata con successo" << std::endl;
		return true;
	}
}

//Imposta i parametri di scambio del canale
static int SetComParms(HANDLE* handleCom, int nComRate, int nComBits, COMMTIMEOUTS timeout) {
	DCB dcb;										// Windows device control block
	// Clear DCB to start out clean, then get current settings
	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);
	if (!GetCommState(*handleCom, &dcb))
		return(0);

	// Set our own parameters from Globals
	dcb.BaudRate = nComRate;						// Baud (bit) rate
	dcb.ByteSize = (BYTE)nComBits;					// Number of bits(8)
	dcb.Parity = 0;									// No parity	
	dcb.StopBits = ONESTOPBIT;						// One stop bit
	if (!SetCommState(*handleCom, &dcb))
		return(0);

	// Set communication timeouts (SEE COMMTIMEOUTS structure in MSDN) - want a fairly long timeout
	memset((void*)&timeout, 0, sizeof(timeout));
	timeout.ReadIntervalTimeout = 500;				// Maximum time allowed to elapse before arival of next byte in milliseconds. If the interval between the arrival of any two bytes exceeds this amount, the ReadFile operation is completed and buffered data is returned
	timeout.ReadTotalTimeoutMultiplier = 1;			// The multiplier used to calculate the total time-out period for read operations in milliseconds. For each read operation this value is multiplied by the requested number of bytes to be read
	timeout.ReadTotalTimeoutConstant = 5000;		// A constant added to the calculation of the total time-out period. This constant is added to the resulting product of the ReadTotalTimeoutMultiplier and the number of bytes (above).
	SetCommTimeouts(*handleCom, &timeout);
	return(1);
}

//Invio dati sulla COM (Scrive sul canale come fosse un file di testo)
void outputToPort(HANDLE* handleCom, LPCVOID buf, DWORD szBuf) {
	int i = 0;
	DWORD NumberofBytesTransmitted;
	LPDWORD lpErrors = 0;
	LPCOMSTAT lpStat = 0;

	i = WriteFile(
		*handleCom,									// Write handle pointing to COM port
		buf,										// Buffer size
		szBuf,										// Size of buffer
		&NumberofBytesTransmitted,					// Written number of bytes
		NULL
	);
	// Handle the timeout error
	if (i == 0) {
		std::cout << "\nWrite Error: " << GetLastError() << std::endl;

		ClearCommError(handleCom, lpErrors, lpStat);		// Clears the device error flag to enable additional input and output operations. Retrieves information ofthe communications error.	
	}
	else
		std::cout << "\nSuccessful transmission, there were :" << NumberofBytesTransmitted << " bytes transmitted" << std::endl;

}

DWORD inputFromPort(HANDLE* hCom, LPVOID buf, DWORD szBuf) {
	int i = 0;
	DWORD NumberofBytesRead;
	LPDWORD lpErrors = 0;
	LPCOMSTAT lpStat = 0;

	i = ReadFile(
		*hCom,										// Read handle pointing to COM port
		buf,										// Buffer size
		szBuf,  									// Size of buffer - Maximum number of bytes to read
		&NumberofBytesRead,
		NULL
	);
	if (i == 0) {
		std::cout << "\nRead Error: " << GetLastError() << std::endl;

		ClearCommError(hCom, lpErrors, lpStat);		// Clears the device error flag to enable additional input and output operations. Retrieves information ofthe communications error.
	}
	else
		std::cout << "\nSuccessful reception!, There were: " << NumberofBytesRead << std::endl;


	return(NumberofBytesRead);
}

void purgePort(HANDLE* handleCom) {
	PurgeComm(*handleCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
}

bool sendCoordinats(HANDLE* handleCom, std::string coordinate) {
	const int BUFSIZE = coordinate.length();

	DWORD bytesRead;

	outputToPort(handleCom, coordinate.c_str(), BUFSIZE);
	return true;
}