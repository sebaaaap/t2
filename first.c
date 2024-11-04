#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define NUM_PROFESORES 12
#define NUM_SEMANAS 4
#define MAX_SERIES 60

typedef struct {
    char nombre[50];
} Serie;

typedef struct {
    char nombre[20];
    Serie series[MAX_SERIES];
    int num_series;
    pthread_mutex_t mutex;
    int series_vistas[NUM_SEMANAS][MAX_SERIES]; // Array para rastrear series vistas por semana
} Plataforma;

typedef struct {
    char nombre[50];
    Plataforma* plataforma_asociada;
    float series_por_semana; 
    float num_series_vistas;
} Profesor;

int contador_profesores = 0;
pthread_mutex_t contador_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t semana_completada = PTHREAD_COND_INITIALIZER;

int generarSeriesPorSemana() {
    return rand() % 6 + 10;
}

float seriesVistasPorSemana() {
    float opciones[] = {0.5, 1.0, 1.5, 2.0};
    return opciones[rand() % 4];
}

void acumularSeries(Plataforma* plataforma) {
    pthread_mutex_lock(&plataforma->mutex);
    int nuevas_series = generarSeriesPorSemana();
    for (int i = 0; i < nuevas_series; i++) {
        if (plataforma->num_series < MAX_SERIES) {
            int num_serie = plataforma->num_series + 1;
            sprintf(plataforma->series[plataforma->num_series].nombre, "%s Serie %d", plataforma->nombre, num_serie);
            plataforma->num_series++;
        }
    }
    pthread_mutex_unlock(&plataforma->mutex);
}

void verserie(Profesor* profesor, int semana) {
    Plataforma* plataforma = profesor->plataforma_asociada;
    pthread_mutex_lock(&plataforma->mutex);

    int max_series_disponibles = (semana + 1) * 15;
    if (plataforma->num_series < max_series_disponibles) {
        max_series_disponibles = plataforma->num_series;
    }

    int i;
    for (i = rand() % max_series_disponibles; plataforma->series_vistas[semana][i] == 1; i = rand() % max_series_disponibles);
    // Marca la serie como vista para la semana actual
    plataforma->series_vistas[semana][i] = 1;

    Serie serie = plataforma->series[i];
    float cant_ver = (profesor->series_por_semana >= 1) ? 1 : 0.5;
    profesor->series_por_semana -= cant_ver;
    profesor->num_series_vistas += cant_ver;

    
        printf("%s vio %.1f de la serie %s (Semana %d).\n", profesor->nombre, cant_ver, serie.nombre, semana + 1);
    

    pthread_mutex_unlock(&plataforma->mutex);
}

void trabajar(Profesor* profesor) {
    printf("%s está trabajando...\n", profesor->nombre);
    usleep(100000);
}

void barrera_semana(int semana) {
    pthread_mutex_lock(&contador_mutex);
    contador_profesores++;
    if (contador_profesores < NUM_PROFESORES) {
        pthread_cond_wait(&semana_completada, &contador_mutex);
    } else {
        contador_profesores = 0;
        printf("\n--- Semana %d completada ---\n", semana + 1);
        pthread_cond_broadcast(&semana_completada);
    }
    pthread_mutex_unlock(&contador_mutex);
}

void* actividadProfesor(void* arg) {
    Profesor* profesor = (Profesor*)arg;
    for (int semana = 0; semana < NUM_SEMANAS; semana++) {
        trabajar(profesor);
        profesor->series_por_semana = seriesVistasPorSemana();
        if (profesor->series_por_semana < 0.5) {
            profesor->series_por_semana = 0.5;
        }
        while (profesor->series_por_semana > 0) {
            verserie(profesor, semana);
        }
        barrera_semana(semana);
    }
    return NULL;
}

void mostrarEstadisticas(Profesor profesores[], Plataforma dasney, Plataforma betflix) {
    printf("\n--- Estadísticas Finales ---\n");
    printf("\nTotal de series en Dasney: %d\n", dasney.num_series);
    for (int i = 0; i < dasney.num_series; i++) printf("  %s\n", dasney.series[i].nombre);
    printf("\nTotal de series en Betflix: %d\n", betflix.num_series);
    for (int i = 0; i < betflix.num_series; i++) printf("  %s\n", betflix.series[i].nombre);
    for (int i = 0; i < NUM_PROFESORES; i++) printf("\n%s vio un total de %.1f series.\n", profesores[i].nombre, profesores[i].num_series_vistas);
}

int main() {
    Plataforma dasney = {"Dasney", {}, 0, PTHREAD_MUTEX_INITIALIZER, {{0}}}; 
    Plataforma betflix = {"Betflix", {}, 0, PTHREAD_MUTEX_INITIALIZER, {{0}}};
    
    Profesor profesores[NUM_PROFESORES];
    pthread_t hilos[NUM_PROFESORES];

    for (int i = 0; i < NUM_PROFESORES; i++) {
        sprintf(profesores[i].nombre, "Profesor %d", i + 1);
        profesores[i].plataforma_asociada = (i < 6) ? &dasney : &betflix;
        profesores[i].series_por_semana = 2.0;
        profesores[i].num_series_vistas = 0.0; 
        pthread_create(&hilos[i], NULL, actividadProfesor, &profesores[i]);
    }

    for (int semana = 0; semana < NUM_SEMANAS; semana++) {
        acumularSeries(&dasney);
        acumularSeries(&betflix);
        usleep(100000);
    }

    for (int i = 0; i < NUM_PROFESORES; i++) pthread_join(hilos[i], NULL);

    mostrarEstadisticas(profesores, dasney, betflix);

    pthread_mutex_destroy(&dasney.mutex);
    pthread_mutex_destroy(&betflix.mutex);
    pthread_mutex_destroy(&contador_mutex);
    pthread_cond_destroy(&semana_completada);

    return 0;
}
