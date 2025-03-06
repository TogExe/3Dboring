//
// Created by TogExe on 03/03/2025.
//
// donjon.h

#ifndef DONJON_H
#define DONJON_H

#define DONJON_LARGEUR 100
#define DONJON_HAUTEUR 100

typedef struct Salle{
    int x, y, largeur, hauteur;
} Salle;

Salle generer_salle();
int salles_se_chevauchent(Salle salle1, Salle salle2);
void deplacer_ver_centre(Salle * salles, int nb);
Salle* generer_salles(int nombre_salles);
void creer_couloir_horizontal(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR], int x1, int y, int x2);
void creer_couloir_vertical(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR], int x, int y1, int y2);
void creer_couloir(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR], Salle salle1, Salle salle2);
void generer_donjon(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR]);

#endif // DONJON_H
