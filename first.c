#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>



//aqui se ajustan los profesores, semanas y numero de series
#define NUM_PROFESORES 12
#define NUM_SEMANAS 4 // 4 semanas(1mes), 26 semanas(6meses) y 52 semanas(1año)
#define MAX_SERIES 60 // 60 series(1mes), 390 series(6meses) y 780 series(1año)


//estructura de una serie
typedef struct {
    char nombre[50];
} Serie;

//estructura de una Plataforma
typedef struct {
    char nombre[20];
    Serie series[MAX_SERIES]; //series de la plataforma
    int num_series;
    pthread_mutex_t mutex; //mutex para permitir el acceso a la plataforma
} Plataforma;

//estructura de un profesor
typedef struct {
    char nombre[50];
    Plataforma* plataforma_asociada; //cada profe es asociado a una plataforma
    float series_por_semana; 
    float num_series_vistas;
} Profesor;



// mutex y condiciones para hacer un print xddd
// "barrerra_semanal"  se llama, alli lo ocupamos 
int contador_profesores = 0;
pthread_mutex_t contador_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t semana_completada = PTHREAD_COND_INITIALIZER;

//generador entre 10 a 15 series por semana
int generarSeriesPorSemana() {
    return rand() % 6 + 10;
}

//elije aleatoriamente las series vista por un profe
float seriesVistasPorSemana() {
    float opciones[] = {0.5, 1.0, 1.5, 2.0};
    return opciones[rand() % 4];
}


// Añade nuevas series a la plataforma semanalmente
void acumularSeries(Plataforma* plataforma) {
    pthread_mutex_lock(&plataforma->mutex);

    //asigna nombre unicos a las series
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


//simula que se esta viendo una serie
void verserie(Profesor* profesor, int semana) {

    pthread_mutex_lock(&profesor->plataforma_asociada->mutex);//ingresa a la plataforma

    int max_series_disponibles = (semana + 1) * 15; // limita las series que se pueden ver en la semana actual
    if (profesor->plataforma_asociada->num_series < max_series_disponibles) {
        max_series_disponibles = profesor->plataforma_asociada->num_series; // Ajustar si hay menos series de las esperadas
    }

    int i = rand() % max_series_disponibles; //genera un numero ramdom
    Serie serie = profesor->plataforma_asociada->series[i]; //rescata una serie en la plataforma con ese indice "i"
    float cant_ver = (profesor->series_por_semana >= 1) ? 1 : 0.5; // se determina si ve 0,5 o 1

    profesor->series_por_semana -= cant_ver; // disminuye la cant que puede ver el profe, max 2
    profesor->num_series_vistas += cant_ver; // aumenta la cant vistas
    if (cant_ver == 1.0) {
        printf("%s vio completa la serie %s (Semana %d).\n", profesor->nombre, serie.nombre, semana + 1);
    } else {
        printf("%s vio %.1f de la serie %s (Semana %d).\n", profesor->nombre, cant_ver, serie.nombre, semana + 1);
    }

    pthread_mutex_unlock(&profesor->plataforma_asociada->mutex); //el profe ya dejo de ver su serie, permite el acceso a los demas
}

//simula jornada laboral    
void trabajar(Profesor* profesor) {
    printf("%s está trabajando...\n", profesor->nombre);
    usleep(100000);
}

//esto mas que nada para que salga el print "semana completada"
//totalmente innecesario pero queda mas entendible el output
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

//simula todo lo que hace un profe en la semana
void* actividadProfesor(void* arg) {
    Profesor* profesor = (Profesor*)arg;

    for (int semana = 0; semana < NUM_SEMANAS; semana++) {
        trabajar(profesor); //trabaja

        profesor->series_por_semana = seriesVistasPorSemana(); //elije una cant ramdom de series a ver entre 0,5 y 2
        
        //asegura que todos los profes vean algo, minimo 0,5 x semana
        if (profesor->series_por_semana < 0.5) {
            profesor->series_por_semana = 0.5;
        }
        
        while (profesor->series_por_semana > 0) {
            verserie(profesor, semana); // se van a ver series los que pueden 
        }
        barrera_semana(semana); // una ves pasado cada profe
    }
    return NULL;
}

//Muestra las estadisticas
void mostrarEstadisticas(Profesor profesores[], Plataforma dasney, Plataforma betflix) {
    printf("\n--- Estadísticas Finales ---\n");
    printf("\nTotal de series en Dasney: %d\n", dasney.num_series);
    for (int i = 0; i < dasney.num_series; i++) printf("  %s\n", dasney.series[i].nombre);
    printf("\nTotal de series en Betflix: %d\n", betflix.num_series);
    for (int i = 0; i < betflix.num_series; i++) printf("  %s\n", betflix.series[i].nombre);
    for (int i = 0; i < NUM_PROFESORES; i++) printf("\n%s vio un total de %.1f series.\n", profesores[i].nombre, profesores[i].num_series_vistas);
}

int main() {
    
    
    //inicia los mutex de las plataformas
    Plataforma dasney = {"Dasney", {}, 0, PTHREAD_MUTEX_INITIALIZER}; 
    Plataforma betflix = {"Betflix", {}, 0, PTHREAD_MUTEX_INITIALIZER};
    
    Profesor profesores[NUM_PROFESORES]; //array de objetos profes
    pthread_t hilos[NUM_PROFESORES]; // hilos que van a simular los profes

    for (int i = 0; i < NUM_PROFESORES; i++) {
        sprintf(profesores[i].nombre, "Profesor %d", i + 1);
        profesores[i].plataforma_asociada = (i < 6) ? &dasney : &betflix; // manda 6 y 6 a cada plataforma
        profesores[i].series_por_semana = 2.0; // cada profe como max puede ver 2 series a la semana
        profesores[i].num_series_vistas = 0.0; 
        pthread_create(&hilos[i], NULL, actividadProfesor, &profesores[i]); //
    }

    //cada semana se crean entre 10-15 series por plataforma
    for (int semana = 0; semana < NUM_SEMANAS; semana++) {
        acumularSeries(&dasney);
        acumularSeries(&betflix);
        usleep(100000);
    }

    //espera a que todos terminen
    for (int i = 0; i < NUM_PROFESORES; i++) pthread_join(hilos[i], NULL);

    //llama a la funcion para mostrar las estaditicas
    mostrarEstadisticas(profesores, dasney, betflix);

    //liberamos recursos
    pthread_mutex_destroy(&dasney.mutex);
    pthread_mutex_destroy(&betflix.mutex);
    pthread_mutex_destroy(&contador_mutex);
    pthread_cond_destroy(&semana_completada);

    return 0;
}