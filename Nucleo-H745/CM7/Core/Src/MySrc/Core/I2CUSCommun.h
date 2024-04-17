#ifndef __I2CUSCOMMUN_H__
#define __I2CUSCOMMUN_H__
	
		enum USCmdes{
	   CUSNone,
	   CUSReq, 	//Interrogation �tat
     CUSStop,		//Arr�te d�tection Ultrasons
	  CUSAV, 		//Active d�tection Ultrasons AV (et arr�te AR)
    CUSAR,  		//Active d�tection Ultrasons AR (et arr�te AV)
    CUSAVAR,     //Active tous US
	  CUSDist,     //D�finit distance de d�tection en cm (1 param. 8 bits)
	  CUSAlt,      //Fait fonctionner US en altern� ou non (1 param 8 bits : 0=non, 1=AV/AR, 2=alt total)
	  CUSGetDistAV,   //Demande distance AV (renvoie 1 octet, en cm. Mesure AV doit �tre active)
	  CUSGetDistAR,   //Demande distance AR (renvoie 1 octet, en cm. Mesure AR doit �tre active)
	  CUSChen,     //Fait un chenillard sur les LEDs
	  CUSGetDistARG,   //Demande distance ARG (renvoie 1 octet, en cm. Mesure AR doit �tre active)
	  CUSGetDistARD,   //Demande distance ARD (renvoie 1 octet, en cm. Mesure AR doit �tre active)
	  CUSGetDistAVG,   //Demande distance AVG (renvoie 1 octet, en cm. Mesure AV doit �tre active)
	  CUSGetDistAVD,   //Demande distance AVD (renvoie 1 octet, en cm. Mesure AV doit �tre active)
	  CUSCouleur,   //Transmets la couleur de jeu
	  CUSSensib,    //R�gle la sensibilit� de mesures (param 1 octet >0)
	  CUSAVGen,     //Active ou d�sactive le capteur AVG ind�pendemment des commandes CUSAV,CUSAR,CUSAVAR
	  CUSAVDen,     //Active ou d�sactive le capteur AVD ind�pendemment des commandes CUSAV,CUSAR,CUSAVAR
	  CUSARGen,     //Active ou d�sactive le capteur ARG ind�pendemment des commandes CUSAV,CUSAR,CUSAVAR
	  CUSARDen,     //Active ou d�sactive le capteur ARD ind�pendemment des commandes CUSAV,CUSAR,CUSAVAR
	  CUSDistAV,     //D�finit distance de d�tection AV en cm (1 param. 8 bits)
	  CUSDistAR,     //D�finit distance de d�tection AR en cm (1 param. 8 bits)
	  CUSTrisOut,    //Met la sortie _RET_CAPTUS en haute imp�dance (1 param 8 bits ON/OFF)
     };

#endif
