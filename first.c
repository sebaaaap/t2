#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


//segundo comit ctm
#define MAX_SERIES 60  // Capacidad máxima de series acumuladas
#define NUM_PROFESORES 12
#define NUM_SEMANAS 4 // Cambia este valor para simular 1 mes (4), 6 meses (26), o 1 año (52)

// Estructura para representar una Serie
typedef struct {
    char nombre[50];
} Serie;

// Estructura para representar una Plataforma
typedef struct {
    char nombre[20];
    Serie series[MAX_SERIES];
    int num_series;
    pthread_mutex_t mutex;
} Plataforma;

// Estructura para representar un Profesor
typedef struct {
    char nombre[50];
    Plataforma* plataforma_asociada;
    float series_por_semana;
    float num_series_vistas;
} Profesor;

pthread_mutex_t print_mutex;  // Mutex para sincronizar la impresión de semanas

// Genera una cantidad de series para la semana (entre 10 y 15)
int generarSeriesPorSemana() {
    return rand() % 6 + 10;
}

// Genera la cantidad de series que un profesor verá en una semana
float seriesVistasPorSemana() {
    float opciones[] = {0.5, 1.0, 1.5, 2.0};
    return opciones[rand() % 4];
}

// Acumula series nuevas en la plataforma cada semana
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

// El profesor ve series según su tiempo asignado por semana
void verserie(Profesor* profesor, int semana) {
    pthread_mutex_lock(&profesor->plataforma_asociada->mutex);

    int i = rand() % profesor->plataforma_asociada->num_series;
    Serie serie = profesor->plataforma_asociada->series[i];
    float cant_ver = (profesor->series_por_semana >= 1) ? 1 : 0.5;

    profesor->series_por_semana -= cant_ver;
    profesor->num_series_vistas += cant_ver;
    if(cant_ver == 1.0) {
        printf("%s vio completa la serie %s (Semana %d).\n", profesor->nombre, serie.nombre, semana + 1);
    } else {
        printf("%s vio %.1f de la serie %s (Semana %d).\n", profesor->nombre, cant_ver, serie.nombre, semana + 1);
    }

    pthread_mutex_unlock(&profesor->plataforma_asociada->mutex);
}

// Simula el tiempo de trabajo del profesor (espera una semana)
void trabajar(Profesor* profesor) {
    printf("%s está trabajando...\n", profesor->nombre);
    usleep(100000);
}

// Actividad que realiza cada profesor en su thread
void* actividadProfesor(void* arg) {
    Profesor* profesor = (Profesor*)arg;

    for (int semana = 0; semana < NUM_SEMANAS; semana++) {
        trabajar(profesor);  // Simula tiempo de trabajo

        // Determinar cuántas series verá el profesor esta semana
        profesor->series_por_semana = seriesVistasPorSemana();

        // Intentar ver series en su plataforma
        while (profesor->series_por_semana > 0) {
            verserie(profesor, semana);
        }
    }
    return NULL;
}

// Muestra estadísticas finales de la simulación
void mostrarEstadisticas(Profesor profesores[], Plataforma dasney, Plataforma betflix) {
    printf("\n--- Estadísticas Finales ---\n");

    printf("\nTotal de series en Dasney: %d\n", dasney.num_series);
    printf("Series acumuladas en Dasney:\n");
    for (int i = 0; i < dasney.num_series; i++) {
        printf("  %s\n", dasney.series[i].nombre);
    }

    printf("\nTotal de series en Betflix: %d\n", betflix.num_series);
    printf("Series acumuladas en Betflix:\n");
    for (int i = 0; i < betflix.num_series; i++) {
        printf("  %s\n", betflix.series[i].nombre);
    }

    for (int i = 0; i < NUM_PROFESORES; i++) {
        printf("\n%s vio un total de %.1f series.\n", profesores[i].nombre, profesores[i].num_series_vistas);
    }
}

int main() {
    srand(time(NULL));
    pthread_mutex_init(&print_mutex, NULL);

    Plataforma dasney = {"Dasney", {}, 0, PTHREAD_MUTEX_INITIALIZER};
    Plataforma betflix = {"Betflix", {}, 0, PTHREAD_MUTEX_INITIALIZER};

    Profesor profesores[NUM_PROFESORES];
    pthread_t hilos[NUM_PROFESORES];

    // Asignar profesores a plataformas
    for (int i = 0; i < NUM_PROFESORES; i++) {
        sprintf(profesores[i].nombre, "Profesor %d", i + 1);
        profesores[i].plataforma_asociada = (i < 6) ? &dasney : &betflix;
        profesores[i].series_por_semana = 2.0;
        profesores[i].num_series_vistas = 0.0;

        pthread_create(&hilos[i], NULL, actividadProfesor, &profesores[i]);
    }

    // Imprimir cada semana solo una vez
    for (int semana = 0; semana < NUM_SEMANAS; semana++) {
        pthread_mutex_lock(&print_mutex);
        printf("\n--- Semana %d ---\n", semana + 1);
        pthread_mutex_unlock(&print_mutex);

        acumularSeries(&dasney);
        acumularSeries(&betflix);
        usleep(100000);
    }

    for (int i = 0; i < NUM_PROFESORES; i++) {
        pthread_join(hilos[i], NULL);
    }

    mostrarEstadisticas(profesores, dasney, betflix);

    pthread_mutex_destroy(&dasney.mutex);
    pthread_mutex_destroy(&betflix.mutex);
    pthread_mutex_destroy(&print_mutex);

    return 0;
}
