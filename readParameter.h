#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>


struct parameters {
    std::string COM_sup;
    int tolerance;                      //Numero di pixel contenenti plastica
    int dB;								//Numero di Righe dell'immagine - considerando che si usa una Bobcat320 il numero di righe e' 320
    int dS;								//Numero di Colonne dell'immagine - considerando che si usa una Bobcat320 il numero di colonne e' 256
    int uB;								//Passo di sottocampionamento delle bande
    int dark;							//bias livello nero
    int nBS;							//Numero di Bande saltate
    std::vector<std::vector<int>> bu;	//lista ordinata delle bande usate per ciascun classificatore: bu matrice di nC righe per nF indici, di valori compresi tra 1 e (dB div uB))
    std::vector<std::vector<float>> W;	//matrice W dei coefficienti delle combinazioni lineari, di dimensione nC * (nF+1)
};
//void stampa: Stampa a video il vettore dei materiali da ricercare nell'immagine spettrale

void stampa(parameters p) { // stampa tutti i parametri caricati da file
    std::cout << "COM= " << p.COM_sup << std::endl;
    std::cout << "dB= " << p.dB << std::endl;
    std::cout << "dS= " << p.dS << std::endl;
    std::cout << "uB= " << p.uB << std::endl;
    std::cout << "dark= " << p.dark << std::endl;
    std::cout << "nBS= " << p.nBS << std::endl;
    for (auto iq = p.bu.begin(); iq != p.bu.end(); iq++) {
        for (auto ic = iq->cbegin(); ic != iq->cend(); ic++) {
            std::cout << *ic << " ";
        }
        std::cout << std::endl;

    }
    std::cout << std::endl;
    for (auto iq = p.W.begin(); iq != p.W.end(); iq++) {
        for (auto ic = iq->cbegin(); ic != iq->cend(); ic++) {
            std::cout << *ic << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

}


//future implementazioni: verifica che i parametri siano idonei
int numParam(std::string s) {

    int n = 0;
    int start = 0;
    int stop = 0;
    int count = 0;
    int endpos = 0;
    endpos = s.find(".", 0);
    while (start != (endpos - 3)) {  // da aggiungere un controllo per l'ultima posizione

        stop = s.find(",", start);
        count = stop - start;
        start = stop + 1;
        n++;
    }

    return n;
}


parameters caricaParametri(char* COMM, int* ftosend) {
    /**************************Dichiarazione variabili**************************/

    const std::string nomefile = "c:\\parametriMateriali.txt";    //nome del file da aprire. aggiungere path per mettere il file in C:?
    std::fstream parametri;
    std::string sup;
    parameters p;                              //variabile di supporto per caricare i materiali dal file e poi aggiungere mat al vettore
    int start, stop, count;
    int endLine = 0;                              //variabili di supporto utilizzata per identificare gli elementi della stringa
    std::vector<int> buElementi;
    std::vector<float> wElementi;
    int n;
    int i = 0;                                  //variabile di supporto
    int j = 0;                                  //variabile di supporto
    int numParametri = 0;                       //variabile di supporto per memorizzare il numero dei parametri di una riga
    std::string line;                           //variabile di supporto per memorizzare la riga prelevata dal file
    std::string parametro;
    /**************************Fine Dichiarazione variabili**************************/

    /**************************Lettura parametri da file**********************************/
    parametri.open(nomefile, std::ios::in);       //Apre il file in modalità lettura -- da aggiungere un try catch?
    if (parametri.is_open()) {                     //esegui le istruzioni sotto se il file è stato aperto correttament
        while (getline(parametri, line)) {         //legge una riga dal file e la memorizza nella variabile line.

            if (line.substr(0, 8) == "Material") {          //se la riga inizia con il commento viene scartata

                while (getline(parametri, line)) {
                    if (line == "end") break;               //il tag end fa uscire dal loop di lettora parametri
                    stop = line.find("=", 0);
                    parametro = line.substr(0, stop);
                    start = stop + 1;
                    if (parametro == "COM") {
                        stop = line.find("\n", start);
                        p.COM_sup = "\\\\.\\" + line.substr(start, stop);
                        for (int i = 0; i < p.COM_sup.length() + 1; i++)
                            COMM[i] = p.COM_sup[i];
                    }
                    else if (parametro == "frequencetosend") {
                        stop = line.find("\n", start);
                        *ftosend = stoi(line.substr(start, stop));
                    }
                    else if (parametro == "dB") {
                        stop = line.find("\n", start);
                        p.dB = stoi(line.substr(start, stop));
                    }
                    else if (parametro == "tolerance") {
                        stop = line.find("\n", start);
                        p.tolerance = stoi(line.substr(start, stop));
                    }
                    else if (parametro == "dS") {
                        stop = line.find("\n", start);
                        p.dS = stoi(line.substr(start, stop));
                    }
                    else if (parametro == "uB") {
                        stop = line.find("\n", start);
                        p.uB = stoi(line.substr(start, stop));
                    }
                    else if (parametro == "dark") {
                        stop = line.find("\n", start);
                        p.dark = stoi(line.substr(start, stop));
                    }
                    
                    else if (parametro == "nBS") {
                        stop = line.find("\n", start);
                        p.nBS = stoi(line.substr(start, stop));
                    }
                    else if (parametro == "bu") {
                        endLine = line.find(".", start);

                        while (stop > 0) {
                            stop = line.find(" ", start) - 1;
                            //if (stop < 0) break;

                            buElementi.push_back(stoi(line.substr(start, stop)));

                            start = stop + 2;

                        }
                        p.bu.push_back(buElementi);
                        buElementi.clear();    

                    }

                    else if (parametro == "W") {
                        endLine = line.find(".", start);

                        while (stop > 0) {
                            stop = line.find(" ", start) - 1;
                            //if (stop < 0) break;

                            wElementi.push_back(std::stof(line.substr(start, stop)));

                            start = stop + 2;

                        }
                        p.W.push_back(wElementi);
                        wElementi.clear();   
                    }


                }

            }


        }
        parametri.close(); //close the file object.
    }
    else { std::cout << "file not found"; }
    return p;

    /**************************Fine Lettura parametri da file**********************************/

}
