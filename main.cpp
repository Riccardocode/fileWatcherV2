#include <iostream>    
#include <fstream>     
#include <sstream>
#include <string>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "FileWatcher.h"
#include "readParameter.h"
#include "COM.h"



// dichiaro funzioni prima del main

bool elaboraFile(std::string filename);
std::string matchFile(std::string filename);
std::vector<std::string> split(std::string txt, char delimiter);

//Dichiarazione variabili globali di supporto
bool plastica = false;
int nplastica = 0;									//Numero immagini con plastica
int frequencetosend = 11;							//Numero di frequeza con il quale vengono inviati gli esiti della plastica. se non viene trovato un valore nel file mantiene questo valore
int frequencesup = 0;
bool plasticflag = false;							// vera se almeno una delle immagini in frequencetosend ha la plastica
int nplasticafrequence = 0;
std::fstream my_file;
std::fstream elaboraImg;
std::string parametri = "";
int Counter = 0;
std::string coordinate = "";
int coordinateInviate = 0;
parameters p;

//Variabili per COM
//char COM[] = "\\\\.\\COM1";
char COM[15];
HANDLE handleCOM;
COMMTIMEOUTS timeout;
int nComRate = 57600;								// Baud (Bit) rate in bits/second 
int nComBits = 8;									// Number of bits per frame
bool hasCOM = false;								//Usato per salvare su file in caso la COM non e' presente

int main() {
	p = caricaParametri(COM, &frequencetosend);
	stampa(p);

	//inizializza la COM, se non la trova scrive i valori su file
	hasCOM = createCOM(&handleCOM, COM);
	if (hasCOM) {
		purgePort(&handleCOM);
		SetComParms(&handleCOM, nComRate, nComBits, timeout);
		purgePort(&handleCOM);

		// Filewatch monitora la cartella di destinazione delle immagini ogni 62 millisecondi per verificare se viene depositata una nuova immagine
		FileWatcher fw{ "c:\\camera_shots\\", std::chrono::milliseconds(62) };

		fw.start([](std::string path_to_watch, FileStatus status) -> void {

			if (!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
				return;
			}
			switch (status) {
			case FileStatus::created:
			{
				std::cout << "File created: " << path_to_watch << '\n';
				if (elaboraFile(path_to_watch)) nplasticafrequence++;
				if (frequencesup < frequencetosend - 1) frequencesup++;
				else {
					my_file.open("C:\\IMUupdated\\gpsUpdate.txt", std::ios::in);
					if (!my_file) std::cout << "No such file";
					else {
						getline(my_file, coordinate);

						if (nplasticafrequence > 0) coordinate = "1," + coordinate + "\n";
						else coordinate = "0," + coordinate + "\n";

						std::cout << "plastica trovata: " << nplasticafrequence << std::endl;

						if (sendCoordinats(&handleCOM, coordinate)) {
							coordinateInviate++;
							std::cout << "ok " << coordinateInviate << std::endl;
							std::cout << coordinate << std::endl;
						}  // invia le coordinate alla COM

						//Reset delle variabili prima di iniziare un nuovo set di immagini da analizzare
						nplasticafrequence = 0;
						frequencesup = 0;
						my_file.close();
						coordinate = "";
					}
				}
				break;
			}
			default:
				break;
			}
			});
	}
	else {
		std::cout << "\nGli esiti della plastica verranno salvati su file" << std::endl;
		// Filewatch monitora la cartella di destinazione delle immagini ogni 62 millisecondi per verificare se viene depositata una nuova immagine
		FileWatcher fw{ "c:\\camera_shots\\", std::chrono::milliseconds(62) };
		fw.start([](std::string path_to_watch, FileStatus status) -> void {

			if (!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
				return;
			}
			switch (status) {
			case FileStatus::created:
			{
				std::cout << "File created: " << path_to_watch << '\n';
				if (elaboraFile(path_to_watch)) nplasticafrequence++;
				if (frequencesup < frequencetosend - 1) frequencesup++;
				else {
					my_file.open("C:\\IMUupdated\\gpsUpdate.txt", std::ios::in);
					if (!my_file) std::cout << "No such file";
					else {
						getline(my_file, coordinate);

						if (nplasticafrequence > 0) coordinate = "1," + coordinate + "\n";
						else coordinate = "0," + coordinate + "\n";

						std::cout << "plastica trovata: " << nplasticafrequence << std::endl;

						elaboraImg.open("C:\\elaboraImg\\elaboraImg.txt", std::ios::app);
						elaboraImg << coordinate;
						std::cout << "ok coordinate salvate " << coordinate << std::endl;
						elaboraImg.close();

						//Reset delle variabili prima di iniziare un nuovo set di immagini da analizzare
						nplasticafrequence = 0;
						frequencesup = 0;
						my_file.close();
						coordinate = "";
					}
				}
				break;
			}
			default:
				break;
			}
			});
	}
}



std::vector<std::string> split(std::string txt, char delimiter) {
	std::istringstream ss(txt);
	std::string token;
	std::vector<std::string> ret;

	while (std::getline(ss, token, delimiter)) {
		ret.push_back(token);
	}

	return ret;
}

std::string matchFile(std::string filename)
{
	int distance = 0;
	int cnt = 6;
	int st1 = 2;
	int st2 = 1;
	int j;
	std::string ret = "";
	std::string path = "c:\\imudata\\";
	auto v1 = split(filename, '-');
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		auto v2 = split(entry.path().string(), '-');
		std::cout << entry.path().string();
		j = 0;
		while (std::stoi(v1[st1 + j]) == std::stoi(v2[st2 + j])) j++;

		/*
		i file IMU letti che sono parecchi minuti indietro rispetto all'immagine, andrebbero spostati in un altra cartella
		In questo modo si risparmiano cicli di CPU non rileggendo sempre gli stessi file
		*/
		distance = abs(std::stoi(v1[8]) - std::stoi(v2[7]));
		if (j == 6 && distance < 80) return entry.path().string();
	}
	return ret;
}

bool elaboraFile(std::string filename)
{	/***************Dichiarazione variabili*****************/
	uint16_t buffer[256][320];				//Matrice che contiene l'immagine letta in binario			
	uint16_t cubo[256][320];				//Matrice con numero di colonne ridotto (320/uB) utilizzata 
	//  per memorizzare le bande scalate di 256 e ridotte del bias dark
	float classi[256][76];					//Matrice che contiene le combinazioni lineari su specifiche bande.
	bool allarme = false;					//Allarme notifica se viene riscontrato uno dei materiali ricercati
	int sense = 0;							//Utilizzata per confrontare ---------------- 
	int i, j, k;								//Variabili di supporto per i cicli for
	int plastic_detected = 0;
	float hyperclassiVet[256];
	std::ofstream hyperclassi;
	/*DEBUG: dichiarazioni dei file ofstreaming e apertura dei file per salvare le diverse matrici in file di testo
	std::ofstream bufferFile;
	std::ofstream cuboFile;
	std::ofstream classiFile;
	bufferFile.open("buffer.txt", std::ios_base::app);
	cuboFile.open("cubo.txt", std::ios_base::app);
	classiFile.open("classi.txt", std::ios_base::app);
	*/
	hyperclassi.open("C:\\elaboraImg\\hyperclassi.txt", std::ios_base::app);
	for (i = 0; i < 256; i++) hyperclassiVet[i] = 0; 
	//Inizializzo a zero la matrice cubo
	for (i = 0; i < 256; i++)
		for (j = 0; j < 320; j++)
			cubo[i][j] = 0;
	//Inizializzo a zero la matrice classi
	for (i = 0; i < 256; i++)
		for (j = 0; j < 76; j++)
			classi[i][j] = 0;
	//Inizializzo a zero la matrice buffer
	for (i = 0; i < 256; i++)
		for (j = 0; j < 320; j++)
			buffer[i][j] = 0;

	std::ifstream is(filename, std::ifstream::binary);
	if (is) {
		//	Leggo il file.G16L semplicemente come binario
		//	contenente interi a 16bit per righe(256 righe da 320 colonne).
		is.read((char*)&buffer, sizeof(buffer));

		/* DEBUG:
		Ciclo usato per creare un file di testo per la matrice Buffer
		for (i = 0; i < 256; i++) {
			for (j = 0; j < 320; j++) {
				bufferFile << buffer[i][j] << " ";
			}
			bufferFile << "\n";
		}
		*/
		for (j = 0; j < int((320 / p.uB) - p.nBS); j++) { // ciclo sulle bande  

			for (i = 0; i < 256; i++) { // ciclo sulla posizione spaziale
				cubo[i][j] = (buffer[i][j * p.uB] / 256) - p.dark;
				/*//cuboFile << cubo[i][j] << " "; DEBUG: usato per creare un file di testo della matrice cubo
				//std::cout << cubo[i][j]<<"  "; DEBUG: usato per creare un file di testo della matrice cubo
				*/
				//std::cout << i << std::endl;
			}
			//std::cout << j << std::endl;
			/*//std::cout << std::endl;			 DEBUG
			//cuboFile << "\n";					 DEBUG
			*/
		}
		sense = 0;
		for (i = 0; i < 256; i++) { // ciclo sulla posizione spaziale
			plastic_detected = 0;
			for (j = 0; j < p.nC; j++) { //ciclo sui classificatori
				classi[i][j] = p.W[j][0];
				
				for (k = 0; k < p.nF; k++) { // ciclo sulle features
					classi[i][j] = classi[i][j] + (p.W[j][k + 1] * (float)(cubo[i][p.bu[j][k]]));
				}
				/*
				//classiFile << classi[i][j] << " "; DEBUG: usato per creare un file di testo della matrice classi
				//std::cout << classi[i][j] << "  "; DEBUG: usato per stampare a video la matrice classi
				*/
				// all'interno dell'if se si lascia allarme = true e si commenta return true e' possibile creare tutta la matrice classi[i][j]
				// se invece si vuole un esecuzione piu' veloce commentare allarme = true e attivare return true
				if (classi[i][j] < 0) {
					
					sense++;
					if (sense > p.tolerance) {
						//OTTIMIZZAZIONE: l'utilizzo diretto del return fa risparmiare molte operazioni alla CPU 
						//pero' la matrice classi non viene costruita.
						// Se si vuole costruire la matrice classi, commentare return true ed attivare allarme = true
						allarme = true;
						plastic_detected = 1;
						//return true;
					}
				}
			}
			if (classi[i][0] < 0) { //TODO:da sistemare per renderlo su piu' classificatori
				hyperclassiVet[i] = 1;

			}
			else {
				hyperclassiVet[i] = 0;
			}
			
			/*
			classiFile << "\n";					 DEBUG:usato per creare uno spazio nel file di testo di matrice classi
			*/
		}
		std::cout << sense << std::endl;
		// quando finisco di processare l'immagine ho una riga dell'hypercube. quindi la scrivo sul file hyperclassi
		for (i = 0; i < 255; i++) {
			hyperclassi << hyperclassiVet[i] << ",";
		}
		hyperclassi << hyperclassiVet[i] << std::endl;
	}
	return allarme;
}

