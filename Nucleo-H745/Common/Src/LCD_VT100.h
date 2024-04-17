/* Gestion terminal vt100 sur RS232C

Codes de caractères spéciaux :
	'\t' : Efface la fin de ligne
	'\r' : Retour en début de ligne
	'\n' : Descend le curseur à la ligne en dessous (même colonne)
	0x08 : Efface le caractère avant le curseur et recule le curseur
	0x90 : alpha
	0x91 : beta
	0x92 : epsilon
	0x93 : sigma
	0x94 : rau
	0x95 : teta
	0x96 : infini
	0x97 : omega
	0x98 : SIGMA
	0x99 : pi
	0x9A : µ
*/

#ifndef __TERM_VT100_H__
#define __TERM_VT100_H__
#include <stdint.h>

#ifndef __SIM__
#include "cmsis_os.h"
//#include "task.h"


void HuartAff_RxCpltCallback(void);
void HuartAff_TxCpltCallback(void);
void HuartAff_ErrorCallback(void);
#endif

#define VT100 1
#define LCD 0


/* lcdinit : Fonction d'initialisation.
   Doit être appelée avant toute autre fonction.
   Sortie sur afficheur LCD si prf=0
   Sortie sur VT100 si prf=1
 */
void lcdinit(int prf);

// autorisation (si != 0) ou interdiction (si = 0) des caractères de contrôle pour sortie VT100
// peut être appelée avant lcdinit().
void VTCtrl(int s);

// lcdputc : Ecriture d'un caractère
void lcdputc(unsigned char x);

// lcdgotoxy : Positionne le curseur à la ligne y (1...4) et colonne x (1...20)
void lcdgotoxy(unsigned char x, unsigned char y);

// renvoie le numéro de la colonne courante (1...20)
unsigned lcdgetx(void);

// renvoie le numéro de la ligne courante (1...4)
unsigned lcdgety(void);

// sauvegarde la position courante du curseur et positionne le curseur en x,y
// renvoie 0 si pas de sauvegarde possible (ne modifie pas la position dans ce cas). Renvoie 1 si Ok.
int lcdpushxy(unsigned char x, unsigned char y);

// Replace le curseur à la dernière position sauvegardée par lcdpusxy()
// renvoie 0 si pas de sauvegarde ou 1 si Ok.
int lcdpopxy(void);


// lcdputs : Ecriture d'une cha�ne de caract�res
void lcdputs(const char *str);

// lcdclrscr : Efface l'écran (curseur revient à l'origine)
void lcdclrscr(void);

// lcdclreol : Efface la fin de la ligne, à partir de la position du curseur
void lcdclreol(void);

// lcdhome : Ramène le curseur à l'origine (Ligne 1, Colonne 1)
void lcdhome(void);

/* lcdcursor : Définit le curseur :
   curs=0 : pas de curseur
   curs=1 : souligné
   curs=2 : clignotant */
void lcdcursor(unsigned char curs);

/* lcdprint : Fonction printf modifiée
Quasi identique à printf avec les formats suivants :
%% : affiche '%'
%c : affiche un caractère
%[.c]s : affiche une chaîne de caractères (c caractères maxi)
%[n][l]u : affiche un entier non signé
%[n][l]x : affiche un entier non signé en hexad�cimal
%[n][l]b : affiche un entier non signé en binaire
%[n][l]d : affiche un entier signé
%[n][.c][*m]U : affiche le résultat de l'entier non signé divisé par m
%[n][.c][*m]D : affiche le résultat de l'entier signé divisé par m
[n] : affichage avec n chiffres
[l] : pour un entier long
[.c] : avec c chiffres derrière la virgule
Exemples :
   lcdprintf("Résultat : %5.1*8D\t\r",x);
      Affiche la valeur de (x/8) sur 5 chiffres, avec 1 chiffre après la virgule
      Efface le reste de la ligne et revient en début de ligne
   lcdprintf("N=%u%u%u%.3s\r\n",c,d,u,unit);
      Affiche "N=" et les valeurs de c, d et u, puis la chaîne unit avec 3 car. maxi.
      Passe ensuite au début de la ligne suivante.
*/
void lcdprintf(const char *fmt,...);

// GetChar : Lecture du clavier de la console VT100
// Renvoie le code du caractère reçu ou -1 si rien
int16_t GetChar(void);
int16_t GetCharTimeOut(uint16_t ms);

//Transfert des caractères de sortie affichage vers la fonction fct
//si fct=0 : sortie affichage normal
void lcdFout(void (*fct)(unsigned char));


//Accès possible aux fonctions étendues :
void VTExtended(uint16_t e);


//Fonctions étendues (pour afficheur tactile)

//S�lection couleur d'écriture :
void lcdTextColor565(uint16_t c);
void lcdTextColor(uint32_t c);

//Sélection couleur de fond :
void lcdBackColor565(uint16_t c);
void lcdBackColor(uint32_t c);

//Sélection de la police :
//Polices disponibles : (avec xx=10,12,14,16,18,22,24,28,48)
// Arialxx, Calibrixx, ComicSansMsxx, CourierNewxx, Deliusxx, Consolasxx
//Largeur fixe si 1er caractère en majuscule.
void lcdSetFont(const char *nf);

//Sélectionne la police N° NumFont de taille Size (valeur la plus proche)
//Si NumFont est invalide, ne fait rien (NumFont=0...lcdGetNbFont()-1)
//0:Arial, 1:Calibri, 2:ComicSansMs, 3:CourierNew, 4:Consolas, 5:Delius
//40 à 45 pour largeur fixe
void lcdSetFontN(uint16_t NumFont, uint16_t Size);


//Définition d'une zone tactile sur l'écran
// (x,y) : coordonnées du coin supérieur gauche de la zone dans le LCD
// mul : coef d'agrandissement en hauteur pour la zone tactile en % (Ex : mul=150 -> h*1,5)
// txt : texte à écrire (utilisation des couleurs courantes
// col565 : couleur du cadre entourant la zone
// id : identifiant de la zone à créer (ou à modifier) de 0 à 15
//Quand une zone est créée, l'appui sur la zone renvoie le code ('A'+id) (à lire avec GetChar)
void lcdDefTouch(int16_t x, int16_t y, uint16_t mul, const char *txt, uint16_t col565, uint16_t id);

//Désactivation de la zone tactile indiquée
void lcdUndefTouch(uint16_t id);

//Définition d'une zone de texte dans le LCD
void lcdDefTextArea(uint16_t xn,uint16_t xm, uint16_t yn, uint16_t ym);

//Trace un rectangle plein dans la couleur indiquée
void lcdRectFull(uint16_t xn,uint16_t xm, uint16_t yn, uint16_t ym, uint16_t col);

//Trace un rectangle dans la couleur indiquée
void lcdRect(uint16_t xn,uint16_t xm, uint16_t yn, uint16_t ym, uint16_t col);

//Autorise le scrolling dans la zone texte
void lcdSetScroll(uint16_t c);

//Permet l'écriture en dehors de la zone texte définie (les positions d'écriture deviennent relatives au LCD entier)
void lcdSetOutWind(uint16_t c);

//Mémorisation de la position courante d'écriture (10 niveaux possibles)
void lcdPushTxtPos(void);

//Récupération de la position d'écriture mémorisée
void lcdPopTxtPos(void);

//fixe la position d'écriture en absolu dans la fenêtre de texte (coordonnées LCD)
void lcdGotoXYa(uint16_t x,uint16_t y);

//Efface tout l'écran
void lcdClearAll(void);

//définit le nombre de pixels vides à placer entre 2 caractères
void lcdSetSpc(uint16_t nb);

//Active ou désactive lemode Incrust.
//En mode incrust, la couleur du fond n'est pas reportée derrière les caractères
void lcdSetIncrust(uint16_t on);



//Affiche une liste de gifs pendant la durée dur (en 1/10s)
//Liste donnée sous la forme "nn1*x1;nn2*x2;nn3; ... nni*xi"
//    nni : Num�ro du gif dans le tableau GIF
//    xi : nombre de répétitions - paramètre otionnel. Si absent : 1 seule fois
// GIFs définis :
//    "3d-fou","3d-oki","3d-oublier","3d-ouf","3d-robot-1","3d-robot-3","3d-robot-5","3d-sueur",
//    "blagueur","fusee1","fusee2","fusee3","grimace","lune1","mdr","robot","robot-2","robot-4","tintin019",
void lcdAffLGif(uint16_t dur, const char *liste);


//===================================================
//Fonctions d'interrogation :
//===================================================

//Demande la largeur du texte transmis, pour la police courante
int16_t lcdGetTextWidth(const char *txt);

//Demande le nombre de pixels du LCD en largeur
int16_t lcdGetPixelWidth(void);

//Demande le nombre de pixels du LCD en hauteur
int16_t lcdGetPixelHeight(void);

//Demande la hauteur de la police courante
int16_t lcdGetPoliceHeight(void);

//Demande nombre de polices définies
int16_t lcdGetNbFont(void);


//Demande nombre de pixels espaces entre caractères
int16_t lcdGetSpc(void);

int16_t lcdGetAbsX(void);
int16_t lcdGetX(void);
int16_t lcdGetAbsY(void);
int16_t lcdGetX(void);

//Lecture état touches tactiles. Renvoie une chaine du type "A" pour touche 0, "B" pour touche 1...
char *lcdGetK(void);

//fixe la position d'écriture en absolu dans la fenêtre graphique
void lcdMoveTo(uint16_t x, uint16_t y);

//trace une ligne jusqu'à la position indiquée dans fenêtre graphique
void lcdLineTo(uint16_t x, uint16_t y);

//trace un pixel dans fenêtre graphique (couleur courante)
void lcdPutPixel(uint16_t x, uint16_t y);

//trace un pixel dans fenêtre graphique dans la couleur indiquée
void lcdPutPixelC(uint16_t x, uint16_t y, uint16_t col565);

//trace un cercle de rayon r dont le centre est la position graphique courante
void lcdCircle(uint16_t r);

//trace un cercle de rayon r dont le centre est (x,y)
void lcdCircleXY(uint16_t x, uint16_t y, uint16_t r);



//Indique la seule tâche autorisée à afficher (id=0 pour toutes)
//Faire lcdTaskAllow(xTaskGetCurrentTaskHandle()) pour autoriser la tâche courante
#ifndef __SIM__
void lcdTaskAllow(TaskHandle_t id);
#endif

#ifndef __COLORS_DEFS__
#define __COLORS_DEFS__

#define C_White 0xFFFFFF
#define C_Cyan 0x00FFFF
#define C_Silver 0xC0C0C0
#define C_Blue 0x0000FF
#define C_Grey 0x808080
#define C_Gray 0x808080
#define C_DarkBlue 0x0000A0
#define C_Black 0x000000
#define C_LightBlue 0xADD8E6
#define C_Orange 0xFFA500
#define C_Purple 0x800080
#define C_Brown 0xA52A2A
#define C_Yellow 0xFFFF00
#define C_Maroon 0x800000
#define C_Lime 0x00FF00
#define C_Green 0x008000
#define C_Magenta 0xFF00FF
#define C_Olive 0x808000
#define C_Night 0x0C090A
#define C_Gunmetal 0x2C3539
#define C_Midnight 0x2B1B17
#define C_Charcoal 0x34282C
#define C_DarkSlateGrey 0x25383C
#define C_Oil 0x3B3131
#define C_BlackCat 0x413839
#define C_Iridium 0x3D3C3A
#define C_BlackEel 0x463E3F
#define C_BlackCow 0x4C4646
#define C_GrayWolf 0x504A4B
#define C_VampireGray 0x565051
#define C_GrayDolphin 0x5C5858
#define C_CarbonGray 0x625D5D
#define C_AshGray 0x666362
#define C_CloudyGray 0x6D6968
#define C_SmokeyGray 0x726E6D
#define C_Gray2 0x736F6E
#define C_Granite 0x837E7C
#define C_BattleshipGray 0x848482
#define C_GrayCloud 0xB6B6B4
#define C_GrayGoose 0xD1D0CE
#define C_Platinum 0xE5E4E2
#define C_MetallicSilver 0xBCC6CC
#define C_BlueGray 0x98AFC7
#define C_LightSlateGray 0x6D7B8D
#define C_SlateGray 0x657383
#define C_JetGray 0x616D7E
#define C_MistBlue 0x646D7E
#define C_MarbleBlue 0x566D7E
#define C_SlateBlue 0x737CA1
#define C_SteelBlue 0x4863A0
#define C_BlueJay 0x2B547E
#define C_DarkSlateBlue 0x2B3856
#define C_MidnightBlue 0x151B54
#define C_NavyBlue 0x000080
#define C_BlueWhale 0x342D7E
#define C_LapisBlue 0x15317E
#define C_DenimDarkBlue 0x151B8D
#define C_EarthBlue 0x0000A0
#define C_CobaltBlue 0x0020C2
#define C_BlueberryBlue 0x0041C2
#define C_SapphireBlue 0x2554C7
#define C_BlueEyes 0x1569C7
#define C_RoyalBlue 0x2B60DE
#define C_BlueOrchid 0x1F45FC
#define C_BlueLotus 0x6960EC
#define C_LightSlateBlue 0x736AFF
#define C_WindowsBlue 0x357EC7
#define C_GlacialBlueIce 0x368BC1
#define C_SilkBlue 0x488AC7
#define C_BlueIvy 0x3090C7
#define C_BlueKoi 0x659EC7
#define C_ColumbiaBlue 0x87AFC7
#define C_BabyBlue 0x95B9C7
#define C_LightSteelBlue 0x728FCE
#define C_OceanBlue 0x2B65EC
#define C_BlueRibbon 0x306EFF
#define C_BlueDress 0x157DEC
#define C_DodgerBlue 0x1589FF
#define C_CornflowerBlue 0x6495ED
#define C_SkyBlue 0x6698FF
#define C_ButterflyBlue 0x38ACEC
#define C_Iceberg 0x56A5EC
#define C_CrystalBlue 0x5CB3FF
#define C_DeepSkyBlue 0x3BB9FF
#define C_DenimBlue 0x79BAEC
#define C_LightSkyBlue 0x82CAFA
#define C_DaySkyBlue 0x82CAFF
#define C_JeansBlue 0xA0CFEC
#define C_BlueAngel 0xB7CEEC
#define C_PastelBlue 0xB4CFEC
#define C_SeaBlue 0xC2DFFF
#define C_PowderBlue 0xC6DEFF
#define C_CoralBlue 0xAFDCEC
#define C_LightBlue2 0xADDFFF
#define C_RobinEggBlue 0xBDEDFF
#define C_PaleBlueLily 0xCFECEC
#define C_LightCyan 0xE0FFFF
#define C_Water 0xEBF4FA
#define C_AliceBlue 0xF0F8FF
#define C_Azure 0xF0FFFF
#define C_LightSlate 0xCCFFFF
#define C_LightAquamarine 0x93FFE8
#define C_ElectricBlue 0x9AFEFF
#define C_Aquamarine 0x7FFFD4
#define C_CyanorAqua 0x00FFFF
#define C_TronBlue 0x7DFDFE
#define C_BlueZircon 0x57FEFF
#define C_BlueLagoon 0x8EEBEC
#define C_Celeste 0x50EBEC
#define C_BlueDiamond 0x4EE2EC
#define C_TiffanyBlue 0x81D8D0
#define C_CyanOpaque 0x92C7C7
#define C_BlueHosta 0x77BFC7
#define C_NorthernLightsBlue 0x78C7C7
#define C_MediumTurquoise 0x48CCCD
#define C_Turquoise 0x43C6DB
#define C_Jellyfish 0x46C7C7
#define C_Bluegreen 0x7BCCB5
#define C_MacawBlueGreen 0x43BFC7
#define C_LightSeaGreen 0x3EA99F
#define C_DarkTurquoise 0x3B9C9C
#define C_SeaTurtleGreen 0x438D80
#define C_MediumAquamarine 0x348781
#define C_GreenishBlue 0x307D7E
#define C_GrayishTurquoise 0x5E7D7E
#define C_BeetleGreen 0x4C787E
#define C_Teal 0x008080
#define C_SeaGreen 0x4E8975
#define C_CamouflageGreen 0x78866B
#define C_b79SageGreen 0x000848
#define C_HazelGreen 0x617C58
#define C_VenomGreen 0x728C00
#define C_FernGreen 0x667C26
#define C_DarkForestGreen 0x254117
#define C_MediumSeaGreen 0x306754
#define C_MediumForestGreen 0x347235
#define C_SeaweedGreen 0x437C17
#define C_PineGreen 0x387C44
#define C_JungleGreen 0x347C2C
#define C_ShamrockGreen 0x347C17
#define C_MediumSpringGreen 0x348017
#define C_ForestGreen 0x4E9258
#define C_GreenOnion 0x6AA121
#define C_SpringGreen 0x4AA02C
#define C_LimeGreen 0x41A317
#define C_CloverGreen 0x3EA055
#define C_GreenSnake 0x6CBB3C
#define C_AlienGreen 0x6CC417
#define C_GreenApple 0x4CC417
#define C_YellowGreen 0x52D017
#define C_KellyGreen 0x4CC552
#define C_ZombieGreen 0x54C571
#define C_FrogGreen 0x99C68E
#define C_GreenPeas 0x89C35C
#define C_DollarBillGreen 0x85BB65
#define C_DarkSeaGreen 0x8BB381
#define C_IguanaGreen 0x9CB071
#define C_AvocadoGreen 0xB2C248
#define C_PistachioGreen 0x9DC209
#define C_SaladGreen 0xA1C935
#define C_HummingbirdGreen 0x7FE817
#define C_NebulaGreen 0x59E817
#define C_StoplightGoGreen 0x57E964
#define C_AlgaeGreen 0x64E986
#define C_JadeGreen 0x5EFB6E
#define C_Green2 0x00FF00
#define C_EmeraldGreen 0x5FFB17
#define C_LawnGreen 0x87F717
#define C_Chartreuse 0x8AFB17
#define C_DragonGreen 0x6AFB92
#define C_Mintgreen 0x98FF98
#define C_GreenThumb 0xB5EAAA
#define C_LightJade 0xC3FDB8
#define C_TeaGreen 0xCCFB5D
#define C_GreenYellow 0xB1FB17
#define C_SlimeGreen 0xBCE954
#define C_Goldenrod 0xEDDA74
#define C_HarvestGold 0xEDE275
#define C_SunYellow 0xFFE87C
#define C_CornYellow 0xFFF380
#define C_Parchment 0xFFFFC2
#define C_Cream 0xFFFFCC
#define C_LemonChiffon 0xFFF8C6
#define C_Cornsilk 0xFFF8DC
#define C_Beige 0xF5F5DC
#define C_Blonde 0xFBF6D9
#define C_AntiqueWhite 0xFAEBD7
#define C_Champagne 0xF7E7CE
#define C_BlanchedAlmond 0xFFEBCD
#define C_Vanilla 0xF3E5AB
#define C_TanBrown 0xECE5B6
#define C_Peach 0xFFE5B4
#define C_Mustard 0xFFDB58
#define C_RubberDuckyYellow 0xFFD801
#define C_BrightGold 0xFDD017
#define C_Goldenbrown 0xEAC117
#define C_MacaroniandCheese 0xF2BB66
#define C_Saffron 0xFBB917
#define C_Beer 0xFBB117
#define C_Cantaloupe 0xFFA62F
#define C_BeeYellow 0xE9AB17
#define C_BrownSugar 0xE2A76F
#define C_BurlyWood 0xDEB887
#define C_DeepPeach 0xFFCBA4
#define C_GingerBrown 0xC9BE62
#define C_SchoolBusYellow 0xE8A317
#define C_SandyBrown 0xEE9A4D
#define C_FallLeafBrown 0xC8B560
#define C_OrangeGold 0xD4A017
#define C_Sand 0xC2B280
#define C_CookieBrown 0xC7A317
#define C_Caramel 0xC68E17
#define C_Brass 0xB5A642
#define C_Khaki 0xADA96E
#define C_Camelbrown 0xC19A6B
#define C_Bronze 0xCD7F32
#define C_TigerOrange 0xC88141
#define C_Cinnamon 0xC58917
#define C_BulletShell 0xAF9B60
#define C_DarkGoldenrod 0xAF7817
#define C_Copper 0xB87333
#define C_Wood 0x966F33
#define C_OakBrown 0x806517
#define C_Moccasin 0x827839
#define C_ArmyBrown 0x827B60
#define C_Sandstone 0x786D5F
#define C_Mocha 0x493D26
#define C_Taupe 0x483C32
#define C_Coffee 0x6F4E37
#define C_BrownBear 0x835C3B
#define C_RedDirt 0x7F5217
#define C_Sepia 0x7F462C
#define C_OrangeSalmon 0xC47451
#define C_Rust 0xC36241
#define C_RedFox 0xC35817
#define C_Chocolate 0xC85A17
#define C_Sedona 0xCC6600
#define C_PapayaOrange 0xE56717
#define C_HalloweenOrange 0xE66C2C
#define C_PumpkinOrange 0xF87217
#define C_ConstructionConeOrange 0xF87431
#define C_SunriseOrange 0xE67451
#define C_MangoOrange 0xFF8040
#define C_DarkOrange 0xF88017
#define C_Coral 0xFF7F50
#define C_BasketBallOrange 0xF88158
#define C_LightSalmon 0xF9966B
#define C_Tangerine 0xE78A61
#define C_DarkSalmon 0xE18B6B
#define C_LightCoral 0xE77471
#define C_BeanRed 0xF75D59
#define C_ValentineRed 0xE55451
#define C_ShockingOrange 0xE55B3C
#define C_Scarlet 0xFF2400
#define C_RubyRed 0xF62217
#define C_FerrariRed 0xF70D1A
#define C_FireEngineRed 0xF62817
#define C_LavaRed 0xE42217
#define C_LoveRed 0xE41B17
#define C_Grapefruit 0xDC381F
#define C_ChestnutRed 0xC34A2C
#define C_CherryRed 0xC24641
#define C_Mahogany 0xC04000
#define C_ChilliPepper 0xC11B17
#define C_Cranberry 0x9F000F
#define C_RedWine 0x990012
#define C_Burgundy 0x8C001A
#define C_Chestnut 0x954535
#define C_BloodRed 0x7E3517
#define C_Sienna 0x8A4117
#define C_Sangria 0x7E3817
#define C_Firebrick 0x800517
#define C_Maroon2 0x810541
#define C_PlumPie 0x7D0541
#define C_VelvetMaroon 0x7E354D
#define C_PlumVelvet 0x7D0552
#define C_RosyFinch 0x7F4E52
#define C_Puce 0x7F5A58
#define C_DullPurple 0x7F525D
#define C_RosyBrown 0xB38481
#define C_KhakiRose 0xC5908E
#define C_PinkBow 0xC48189
#define C_LipstickPink 0xC48793
#define C_Rose 0xE8ADAA
#define C_RoseGold 0xECC5C0
#define C_DesertSand 0xEDC9AF
#define C_PigPink 0xFDD7E4
#define C_CottonCandy 0xFCDFFF
#define C_PinkBubblegum 0xFFDFDD
#define C_MistyRose 0xFBBBB9
#define C_Pink 0xFAAFBE
#define C_LightPink 0xFAAFBA
#define C_FlamingoPink 0xF9A7B0
#define C_PinkRose 0xE7A1B0
#define C_PinkDaisy 0xE799A3
#define C_CadillacPink 0xE38AAE
#define C_CarnationPink 0xF778A1
#define C_BlushRed 0xE56E94
#define C_HotPink 0xF660AB
#define C_WatermelonPink 0xFC6C85
#define C_VioletRed 0xF6358A
#define C_DeepPink 0xF52887
#define C_PinkCupcake 0xE45E9D
#define C_PinkLemonade 0xE4287C
#define C_NeonPink 0xF535AA
#define C_DimorphothecaMagenta 0xE3319D
#define C_BrightNeonPink 0xF433FF
#define C_PaleVioletRed 0xD16587
#define C_TulipPink 0xC25A7C
#define C_MediumVioletRed 0xCA226B
#define C_RoguePink 0xC12869
#define C_BurntPink 0xC12267
#define C_BashfulPink 0xC25283
#define C_DarkCarnationPink 0xC12283
#define C_Plum 0xB93B8F
#define C_ViolaPurple 0x7E587E
#define C_PurpleIris 0x571B7E
#define C_PlumPurple 0x583759
#define C_Indigo 0x4B0082
#define C_PurpleMonster 0x461B7E
#define C_PurpleHaze 0x4E387E
#define C_Eggplant 0x614051
#define C_Grape 0x5E5A80
#define C_PurpleJam 0x6A287E
#define C_DarkOrchid 0x7D1B7E
#define C_PurpleFlower 0xA74AC7
#define C_MediumOrchid 0xB048B5
#define C_PurpleAmethyst 0x6C2DC7
#define C_DarkViolet 0x842DCE
#define C_Violet 0x8D38C9
#define C_PurpleSageBush 0x7A5DC7
#define C_LovelyPurple 0x7F38EC
#define C_Purple2 0x8E35EF
#define C_AztechPurple 0x893BFF
#define C_MediumPurple 0x8467D7
#define C_JasminePurple 0xA23BEC
#define C_PurpleDaffodil 0xB041FF
#define C_TyrianPurple 0xC45AEC
#define C_CrocusPurple 0x9172EC
#define C_PurpleMimosa 0x9E7BFF
#define C_HeliotropePurple 0xD462FF
#define C_Crimson 0xE238EC
#define C_PurpleDragon 0xC38EC7
#define C_Lilac 0xC8A2C8
#define C_BlushPink 0xE6A9EC
#define C_Mauve 0xE0B0FF
#define C_WisteriaPurple 0xC6AEC7
#define C_BlossomPink 0xF9B7FF
#define C_Thistle 0xD2B9D3
#define C_Periwinkle 0xE9CFEC
#define C_LavenderPinocchio 0xEBDDE2
#define C_Lavenderblue 0xE3E4FA
#define C_Pearl 0xFDEEF4
#define C_SeaShell 0xFFF5EE
#define C_MilkWhite 0xFEFCFF

#define C565_White 0xFFFF
#define C565_Cyan 0x07FF
#define C565_Silver 0xC618
#define C565_Blue 0x001F
#define C565_Grey 0x8410
#define C565_Gray 0x8410
#define C565_DarkBlue 0x0014
#define C565_Black 0x0000
#define C565_LightBlue 0xAEDC
#define C565_Orange 0xFD20
#define C565_Purple 0x8010
#define C565_Brown 0xA145
#define C565_Yellow 0xFFE0
#define C565_Maroon 0x8000
#define C565_Lime 0x07E0
#define C565_Green 0x0400
#define C565_Magenta 0xF81F
#define C565_Olive 0x8400
#define C565_Night 0x0841
#define C565_Gunmetal 0x29A7
#define C565_Midnight 0x28C2
#define C565_Charcoal 0x3145
#define C565_DarkSlateGrey 0x21C7
#define C565_Oil 0x3986
#define C565_BlackCat 0x41C7
#define C565_Iridium 0x39E7
#define C565_BlackEel 0x41E7
#define C565_BlackCow 0x4A28
#define C565_GrayWolf 0x5249
#define C565_VampireGray 0x528A
#define C565_GrayDolphin 0x5ACB
#define C565_CarbonGray 0x62EB
#define C565_AshGray 0x630C
#define C565_CloudyGray 0x6B4D
#define C565_SmokeyGray 0x736D
#define C565_Gray2 0x736D
#define C565_Granite 0x83EF
#define C565_BattleshipGray 0x8430
#define C565_GrayCloud 0xB5B6
#define C565_GrayGoose 0xD699
#define C565_Platinum 0xE73C
#define C565_MetallicSilver 0xBE39
#define C565_BlueGray 0x9D78
#define C565_LightSlateGray 0x6BD1
#define C565_SlateGray 0x6390
#define C565_JetGray 0x636F
#define C565_MistBlue 0x636F
#define C565_MarbleBlue 0x536F
#define C565_SlateBlue 0x73F4
#define C565_SteelBlue 0x4B14
#define C565_BlueJay 0x2AAF
#define C565_DarkSlateBlue 0x29CA
#define C565_MidnightBlue 0x10CA
#define C565_NavyBlue 0x0010
#define C565_BlueWhale 0x316F
#define C565_LapisBlue 0x118F
#define C565_DenimDarkBlue 0x10D1
#define C565_EarthBlue 0x0014
#define C565_CobaltBlue 0x0118
#define C565_BlueberryBlue 0x0218
#define C565_SapphireBlue 0x22B8
#define C565_BlueEyes 0x1358
#define C565_RoyalBlue 0x2B1B
#define C565_BlueOrchid 0x1A3F
#define C565_BlueLotus 0x6B1D
#define C565_LightSlateBlue 0x735F
#define C565_WindowsBlue 0x33F8
#define C565_GlacialBlueIce 0x3458
#define C565_SilkBlue 0x4C58
#define C565_BlueIvy 0x3498
#define C565_BlueKoi 0x64F8
#define C565_ColumbiaBlue 0x8578
#define C565_BabyBlue 0x95D8
#define C565_LightSteelBlue 0x7479
#define C565_OceanBlue 0x2B3D
#define C565_BlueRibbon 0x337F
#define C565_BlueDress 0x13FD
#define C565_DodgerBlue 0x145F
#define C565_CornflowerBlue 0x64BD
#define C565_SkyBlue 0x64DF
#define C565_ButterflyBlue 0x3D7D
#define C565_Iceberg 0x553D
#define C565_CrystalBlue 0x5D9F
#define C565_DeepSkyBlue 0x3DDF
#define C565_DenimBlue 0x7DDD
#define C565_LightSkyBlue 0x865F
#define C565_DaySkyBlue 0x865F
#define C565_JeansBlue 0xA67D
#define C565_BlueAngel 0xB67D
#define C565_PastelBlue 0xB67D
#define C565_SeaBlue 0xC6FF
#define C565_PowderBlue 0xC6FF
#define C565_CoralBlue 0xAEFD
#define C565_LightBlue2 0xAEFF
#define C565_RobinEggBlue 0xBF7F
#define C565_PaleBlueLily 0xCF7D
#define C565_LightCyan 0xE7FF
#define C565_Water 0xEFBF
#define C565_AliceBlue 0xF7DF
#define C565_Azure 0xF7FF
#define C565_LightSlate 0xCFFF
#define C565_LightAquamarine 0x97FD
#define C565_ElectricBlue 0x9FFF
#define C565_Aquamarine 0x7FFA
#define C565_CyanorAqua 0x07FF
#define C565_TronBlue 0x7FFF
#define C565_BlueZircon 0x57FF
#define C565_BlueLagoon 0x8F5D
#define C565_Celeste 0x575D
#define C565_BlueDiamond 0x4F1D
#define C565_TiffanyBlue 0x86DA
#define C565_CyanOpaque 0x9638
#define C565_BlueHosta 0x75F8
#define C565_NorthernLightsBlue 0x7E38
#define C565_MediumTurquoise 0x4E79
#define C565_Turquoise 0x463B
#define C565_Jellyfish 0x4638
#define C565_Bluegreen 0x7E76
#define C565_MacawBlueGreen 0x45F8
#define C565_LightSeaGreen 0x3D53
#define C565_DarkTurquoise 0x3CF3
#define C565_SeaTurtleGreen 0x4470
#define C565_MediumAquamarine 0x3430
#define C565_GreenishBlue 0x33EF
#define C565_GrayishTurquoise 0x5BEF
#define C565_BeetleGreen 0x4BCF
#define C565_Teal 0x0410
#define C565_SeaGreen 0x4C4E
#define C565_CamouflageGreen 0x7C2D
#define C565_b79SageGreen 0x0049
#define C565_HazelGreen 0x63EB
#define C565_VenomGreen 0x7460
#define C565_FernGreen 0x63E4
#define C565_DarkForestGreen 0x2202
#define C565_MediumSeaGreen 0x332A
#define C565_MediumForestGreen 0x3386
#define C565_SeaweedGreen 0x43E2
#define C565_PineGreen 0x3BE8
#define C565_JungleGreen 0x33E5
#define C565_ShamrockGreen 0x33E2
#define C565_MediumSpringGreen 0x3402
#define C565_ForestGreen 0x4C8B
#define C565_GreenOnion 0x6D04
#define C565_SpringGreen 0x4D05
#define C565_LimeGreen 0x4502
#define C565_CloverGreen 0x3D0A
#define C565_GreenSnake 0x6DC7
#define C565_AlienGreen 0x6E22
#define C565_GreenApple 0x4E22
#define C565_YellowGreen 0x5682
#define C565_KellyGreen 0x4E2A
#define C565_ZombieGreen 0x562E
#define C565_FrogGreen 0x9E31
#define C565_GreenPeas 0x8E0B
#define C565_DollarBillGreen 0x85CC
#define C565_DarkSeaGreen 0x8D90
#define C565_IguanaGreen 0x9D8E
#define C565_AvocadoGreen 0xB609
#define C565_PistachioGreen 0x9E01
#define C565_SaladGreen 0xA646
#define C565_HummingbirdGreen 0x7F42
#define C565_NebulaGreen 0x5F42
#define C565_StoplightGoGreen 0x574C
#define C565_AlgaeGreen 0x6750
#define C565_JadeGreen 0x5FCD
#define C565_Green2 0x07E0
#define C565_EmeraldGreen 0x5FC2
#define C565_LawnGreen 0x87A2
#define C565_Chartreuse 0x8FC2
#define C565_DragonGreen 0x6FD2
#define C565_Mintgreen 0x9FF3
#define C565_GreenThumb 0xB755
#define C565_LightJade 0xC7F7
#define C565_TeaGreen 0xCFCB
#define C565_GreenYellow 0xB7C2
#define C565_SlimeGreen 0xBF4A
#define C565_Goldenrod 0xEECE
#define C565_HarvestGold 0xEF0E
#define C565_SunYellow 0xFF4F
#define C565_CornYellow 0xFF90
#define C565_Parchment 0xFFF8
#define C565_Cream 0xFFF9
#define C565_LemonChiffon 0xFFD8
#define C565_Cornsilk 0xFFDB
#define C565_Beige 0xF7BB
#define C565_Blonde 0xFFBB
#define C565_AntiqueWhite 0xFF5A
#define C565_Champagne 0xF739
#define C565_BlanchedAlmond 0xFF59
#define C565_Vanilla 0xF735
#define C565_TanBrown 0xEF36
#define C565_Peach 0xFF36
#define C565_Mustard 0xFECB
#define C565_RubberDuckyYellow 0xFEC0
#define C565_BrightGold 0xFE82
#define C565_Goldenbrown 0xEE02
#define C565_MacaroniandCheese 0xF5CC
#define C565_Saffron 0xFDC2
#define C565_Beer 0xFD82
#define C565_Cantaloupe 0xFD25
#define C565_BeeYellow 0xED42
#define C565_BrownSugar 0xE52D
#define C565_BurlyWood 0xDDD0
#define C565_DeepPeach 0xFE54
#define C565_GingerBrown 0xCDEC
#define C565_SchoolBusYellow 0xED02
#define C565_SandyBrown 0xECC9
#define C565_FallLeafBrown 0xCDAC
#define C565_OrangeGold 0xD502
#define C565_Sand 0xC590
#define C565_CookieBrown 0xC502
#define C565_Caramel 0xC462
#define C565_Brass 0xB528
#define C565_Khaki 0xAD4D
#define C565_Camelbrown 0xC4CD
#define C565_Bronze 0xCBE6
#define C565_TigerOrange 0xCC08
#define C565_Cinnamon 0xC442
#define C565_BulletShell 0xACCC
#define C565_DarkGoldenrod 0xABC2
#define C565_Copper 0xBB86
#define C565_Wood 0x9366
#define C565_OakBrown 0x8322
#define C565_Moccasin 0x83C7
#define C565_ArmyBrown 0x83CC
#define C565_Sandstone 0x7B6B
#define C565_Mocha 0x49E4
#define C565_Taupe 0x49E6
#define C565_Coffee 0x6A66
#define C565_BrownBear 0x82E7
#define C565_RedDirt 0x7A82
#define C565_Sepia 0x7A25
#define C565_OrangeSalmon 0xC3AA
#define C565_Rust 0xC308
#define C565_RedFox 0xC2C2
#define C565_Chocolate 0xCAC2
#define C565_Sedona 0xCB20
#define C565_PapayaOrange 0xE322
#define C565_HalloweenOrange 0xE365
#define C565_PumpkinOrange 0xFB82
#define C565_ConstructionConeOrange 0xFBA6
#define C565_SunriseOrange 0xE3AA
#define C565_MangoOrange 0xFC08
#define C565_DarkOrange 0xFC02
#define C565_Coral 0xFBEA
#define C565_BasketBallOrange 0xFC0B
#define C565_LightSalmon 0xFCAD
#define C565_Tangerine 0xE44C
#define C565_DarkSalmon 0xE44D
#define C565_LightCoral 0xE3AE
#define C565_BeanRed 0xF2EB
#define C565_ValentineRed 0xE2AA
#define C565_ShockingOrange 0xE2C7
#define C565_Scarlet 0xF920
#define C565_RubyRed 0xF102
#define C565_FerrariRed 0xF063
#define C565_FireEngineRed 0xF142
#define C565_LavaRed 0xE102
#define C565_LoveRed 0xE0C2
#define C565_Grapefruit 0xD9C3
#define C565_ChestnutRed 0xC245
#define C565_CherryRed 0xC228
#define C565_Mahogany 0xC200
#define C565_ChilliPepper 0xC0C2
#define C565_Cranberry 0x9801
#define C565_RedWine 0x9802
#define C565_Burgundy 0x8803
#define C565_Chestnut 0x9226
#define C565_BloodRed 0x79A2
#define C565_Sienna 0x8A02
#define C565_Sangria 0x79C2
#define C565_Firebrick 0x8022
#define C565_Maroon2 0x8028
#define C565_PlumPie 0x7828
#define C565_VelvetMaroon 0x79A9
#define C565_PlumVelvet 0x782A
#define C565_RosyFinch 0x7A6A
#define C565_Puce 0x7ACB
#define C565_DullPurple 0x7A8B
#define C565_RosyBrown 0xB430
#define C565_KhakiRose 0xC491
#define C565_PinkBow 0xC411
#define C565_LipstickPink 0xC432
#define C565_Rose 0xED75
#define C565_RoseGold 0xEE38
#define C565_DesertSand 0xEE55
#define C565_PigPink 0xFEBC
#define C565_CottonCandy 0xFEFF
#define C565_PinkBubblegum 0xFEFB
#define C565_MistyRose 0xFDD7
#define C565_Pink 0xFD77
#define C565_LightPink 0xFD77
#define C565_FlamingoPink 0xFD36
#define C565_PinkRose 0xE516
#define C565_PinkDaisy 0xE4D4
#define C565_CadillacPink 0xE455
#define C565_CarnationPink 0xF3D4
#define C565_BlushRed 0xE372
#define C565_HotPink 0xF315
#define C565_WatermelonPink 0xFB70
#define C565_VioletRed 0xF1B1
#define C565_DeepPink 0xF150
#define C565_PinkCupcake 0xE2F3
#define C565_PinkLemonade 0xE14F
#define C565_NeonPink 0xF1B5
#define C565_DimorphothecaMagenta 0xE193
#define C565_BrightNeonPink 0xF19F
#define C565_PaleVioletRed 0xD330
#define C565_TulipPink 0xC2CF
#define C565_MediumVioletRed 0xC90D
#define C565_RoguePink 0xC14D
#define C565_BurntPink 0xC10C
#define C565_BashfulPink 0xC290
#define C565_DarkCarnationPink 0xC110
#define C565_Plum 0xB9D1
#define C565_ViolaPurple 0x7ACF
#define C565_PurpleIris 0x50CF
#define C565_PlumPurple 0x59AB
#define C565_Indigo 0x4810
#define C565_PurpleMonster 0x40CF
#define C565_PurpleHaze 0x49CF
#define C565_Eggplant 0x620A
#define C565_Grape 0x5AD0
#define C565_PurpleJam 0x694F
#define C565_DarkOrchid 0x78CF
#define C565_PurpleFlower 0xA258
#define C565_MediumOrchid 0xB256
#define C565_PurpleAmethyst 0x6978
#define C565_DarkViolet 0x8179
#define C565_Violet 0x89D9
#define C565_PurpleSageBush 0x7AF8
#define C565_LovelyPurple 0x79DD
#define C565_Purple2 0x89BD
#define C565_AztechPurple 0x89DF
#define C565_MediumPurple 0x833A
#define C565_JasminePurple 0xA1DD
#define C565_PurpleDaffodil 0xB21F
#define C565_TyrianPurple 0xC2DD
#define C565_CrocusPurple 0x939D
#define C565_PurpleMimosa 0x9BDF
#define C565_HeliotropePurple 0xD31F
#define C565_Crimson 0xE1DD
#define C565_PurpleDragon 0xC478
#define C565_Lilac 0xCD19
#define C565_BlushPink 0xE55D
#define C565_Mauve 0xE59F
#define C565_WisteriaPurple 0xC578
#define C565_BlossomPink 0xFDBF
#define C565_Thistle 0xD5DA
#define C565_Periwinkle 0xEE7D
#define C565_LavenderPinocchio 0xEEFC
#define C565_Lavenderblue 0xE73F
#define C565_Pearl 0xFF7E
#define C565_SeaShell 0xFFBD
#define C565_MilkWhite 0xFFFF

#define C565(rgb) (((((rgb)>>19)&0x1F)<<11)|((((rgb)>>10)&0x3F)<<5)|(((rgb)>>3)&0x1F))

#endif



#endif
