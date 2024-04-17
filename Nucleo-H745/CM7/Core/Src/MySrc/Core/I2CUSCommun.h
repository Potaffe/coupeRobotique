#ifndef __I2CUSCOMMUN_H__
#define __I2CUSCOMMUN_H__
	
		enum USCmdes{
	   CUSNone,
	   CUSReq, 	//Interrogation état
     CUSStop,		//Arrête détection Ultrasons
	  CUSAV, 		//Active détection Ultrasons AV (et arrête AR)
    CUSAR,  		//Active détection Ultrasons AR (et arrête AV)
    CUSAVAR,     //Active tous US
	  CUSDist,     //Définit distance de détection en cm (1 param. 8 bits)
	  CUSAlt,      //Fait fonctionner US en alterné ou non (1 param 8 bits : 0=non, 1=AV/AR, 2=alt total)
	  CUSGetDistAV,   //Demande distance AV (renvoie 1 octet, en cm. Mesure AV doit être active)
	  CUSGetDistAR,   //Demande distance AR (renvoie 1 octet, en cm. Mesure AR doit être active)
	  CUSChen,     //Fait un chenillard sur les LEDs
	  CUSGetDistARG,   //Demande distance ARG (renvoie 1 octet, en cm. Mesure AR doit être active)
	  CUSGetDistARD,   //Demande distance ARD (renvoie 1 octet, en cm. Mesure AR doit être active)
	  CUSGetDistAVG,   //Demande distance AVG (renvoie 1 octet, en cm. Mesure AV doit être active)
	  CUSGetDistAVD,   //Demande distance AVD (renvoie 1 octet, en cm. Mesure AV doit être active)
	  CUSCouleur,   //Transmets la couleur de jeu
	  CUSSensib,    //Règle la sensibilité de mesures (param 1 octet >0)
	  CUSAVGen,     //Active ou désactive le capteur AVG indépendemment des commandes CUSAV,CUSAR,CUSAVAR
	  CUSAVDen,     //Active ou désactive le capteur AVD indépendemment des commandes CUSAV,CUSAR,CUSAVAR
	  CUSARGen,     //Active ou désactive le capteur ARG indépendemment des commandes CUSAV,CUSAR,CUSAVAR
	  CUSARDen,     //Active ou désactive le capteur ARD indépendemment des commandes CUSAV,CUSAR,CUSAVAR
	  CUSDistAV,     //Définit distance de détection AV en cm (1 param. 8 bits)
	  CUSDistAR,     //Définit distance de détection AR en cm (1 param. 8 bits)
	  CUSTrisOut,    //Met la sortie _RET_CAPTUS en haute impédance (1 param 8 bits ON/OFF)
     };

#endif
