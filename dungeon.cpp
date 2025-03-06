//
// Created by TogExe on 03/03/2025.
//
// donjon.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "donjon.h"

// Définir les dimensions du donjon
#define DONJON_LARGEUR 100
#define DONJON_HAUTEUR 70

// Définir les tailles minimales et maximales des salles
#define TAILLE_MIN_SALLE 5
#define TAILLE_MAX_SALLE 8

// Définir le nombre de salles
#define NOMBRE_SALLES 7

// Structure pour représenter une salle (inutile déja dans le donjon.h)

// Fonction pour générer une salle aléatoire
Salle generer_salle() {
    Salle salle;
    salle.largeur = TAILLE_MIN_SALLE + rand() % (TAILLE_MAX_SALLE - TAILLE_MIN_SALLE + 1);
    salle.hauteur = TAILLE_MIN_SALLE + rand() % (TAILLE_MAX_SALLE - TAILLE_MIN_SALLE + 1);
    salle.x = 1 + (rand() % (DONJON_LARGEUR - salle.largeur));
    salle.y = 1 + (rand() % (DONJON_HAUTEUR - salle.hauteur));
    return salle;
}

// Fonction pour vérifier si deux salles se chevauchent
int salles_se_chevauchent(Salle salle1, Salle salle2) {
    return !(salle1.x + salle1.largeur < salle2.x || salle1.x > salle2.x + salle2.largeur ||
             salle1.y + salle1.hauteur < salle2.y || salle1.y > salle2.y + salle2.hauteur);
}

void deplacer_ver_centre(Salle * salles, int nb) {
    for(int i = 0; i < nb; i++) {
        if(salles[i].x > 50) {
            salles[i].x = salles[i].x - 1;
            int a = 0;
            for(int j = 0; j < nb; j++) {
                if(!a) {
                    if(salles_se_chevauchent(salles[i], salles[j]) && j != i) {
                        a = 1;
                    }
                }
            }
            if(a) {
                salles[i].x = salles[i].x + 1;
            }
        } else if(salles[i].x < 50) {
            salles[i].x++;
            int a = 0;
            for(int j = 0; j < nb; j++) {
                if(!a) {
                    if(salles_se_chevauchent(salles[i], salles[j]) && j != i) {
                        a = 1;
                    }
                }
            }
            if(a) {
                salles[i].x--;
            }
        }
    }
}

// Fonction pour générer un tableau de salles non chevauchantes
Salle* generer_salles(int nombre_salles) {
    Salle* salles = (Salle*)malloc(nombre_salles * sizeof(Salle));
    for (int i = 0; i < nombre_salles; i++) {
        Salle nouvelle_salle = generer_salle();
        while (1) {
            int chevauchement = 0;
            for (int j = 0; j < i; j++) {
                if (salles_se_chevauchent(nouvelle_salle, salles[j])) {
                    chevauchement = 1;
                    break;
                }
            }
            if (!chevauchement) {
                break;
            }
            nouvelle_salle = generer_salle();
        }
        salles[i] = nouvelle_salle;
        //deplacer_ver_centre(salles, NOMBRE_SALLES);
    }
    return salles;
}

// Fonction pour créer un couloir horizontal
void creer_couloir_horizontal(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR], int x1, int y, int x2) {
    for (int x = x1; x <= x2; x++) {
        donjon[y][x] = 1;
    }
}

// Fonction pour créer un couloir vertical
void creer_couloir_vertical(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR], int x, int y1, int y2) {
    for (int y = y1; y <= y2; y++) {
        donjon[y][x] = 1;
    }
}

// Fonction pour créer un couloir entre deux salles
void creer_couloir(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR], Salle salle1, Salle salle2) {
    int cx1 = salle1.x;
    int cy1 = salle1.y;
    int cx2 = salle2.x;
    int cy2 = salle2.y;

    if (rand() % 2) {
        creer_couloir_horizontal(donjon, cx1, cy1, cx2);
        creer_couloir_vertical(donjon, cx2, cy1, cy2);
    } else {
        creer_couloir_vertical(donjon, cx1, cy1, cy2);
        creer_couloir_horizontal(donjon, cx1, cy2, cx2);
    }
}

// Fonction pour générer le donjon
void generer_donjon(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR]) {
    Salle* salles = generer_salles(NOMBRE_SALLES);
    for (int i = 0; i < NOMBRE_SALLES; i++) {
        Salle salle = salles[i];
        for (int y = salle.y; y < salle.y + salle.hauteur; y++) {
            for (int x = salle.x; x < salle.x + salle.largeur; x++) {
                donjon[y][x] = 1;
            }
        }
    }

    for (int i = 0; i < NOMBRE_SALLES - 1; i++) {
        creer_couloir(donjon, salles[i], salles[i + 1]);
    }

    free(salles);
}
