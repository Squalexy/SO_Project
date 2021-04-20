/*
Projeto realizado por:
    Alexy de Almeida nº2019192123
    José Gonçalves nº2019223292
*/

#include "declarations.h"
#include <string.h>

int *read_content_from_file()
{
    FILE *fptr;
    char line[LINESIZE];
    char *token = NULL;

    int *file_contents = (int *)malloc(ARRAYSIZE * sizeof(int));

    if ((fptr = fopen("config.txt", "r")) == NULL)
    {
        write_logfile("ERROR OPENING FILE");
        exit(1);
    }

    for (int i = 0; i < ARRAYSIZE; i++)
    {
        if (fgets(line, LINESIZE, fptr) != NULL)
        {

            // No linux podem surgir problemas devido ao tamanho da string, isto serve para meter a string com o tamanho exato
            // line[strlen(line) - 2] = '\0';

            if (strchr(line, ',') != NULL)
            {
                token = strtok(line, ", ");
                file_contents[i] = atoi(token);
                i++;
                token = strtok(NULL, ", ");
                file_contents[i] = atoi(token);
            }
            else
            {
                token = strtok(line, ", ");
                file_contents[i] = atoi(token);
            }
        }
    }

    if (file_contents[3] < 3)
    {
        // perror("Error opening file. Number of teams below 3.\n");
        write_logfile("ERROR OPENING FILE, NUMBER");
        exit(1);
    }

    fclose(fptr);
    return file_contents;
}

void print_content_from_file(int *file_contents)
{
    printf("--- CONTENT FROM FILE ---\n");
    for (int i = 0; i < ARRAYSIZE; i++)
    {
        if (i == 0)
            printf("TIME UNITS : %d\n", file_contents[i]);
        else if (i == 1)
            printf("TURN DISTANCE : %d\n", file_contents[i]);
        else if (i == 2)
            printf("TURNS NUMBER : %d\n", file_contents[i]);
        else if (i == 3)
            printf("TEAMS NUMBER : %d\n", file_contents[i]);
        else if (i == 4)
            printf("CARS LIMIT : %d\n", file_contents[i]);
        else if (i == 5)
            printf("T_AVARIA : %d\n", file_contents[i]);
        else if (i == 6)
            printf("T_BOX_MIN : %d\n", file_contents[i]);
        else if (i == 7)
            printf("T_BOX_MAX : %d\n", file_contents[i]);
        else if (i == 8)
            printf("FUEL CAPACITY : %d\n", file_contents[i]);
    }
    printf("\n");
}

void write_logfile(char *text_to_write)
{
    // FILE *fptr;
    // fptr = fopen("log.txt", "a");
    if (fptr == NULL)
    {
        perror("Error opening file.\n");
        exit(1);
    }

    int hours, minutes, seconds;
    time_t now = time(NULL);

    struct tm *local = localtime(&now);
    hours = local->tm_hour;
    minutes = local->tm_min;
    seconds = local->tm_sec;

    sem_wait(writing);
    printf("%02d:%02d:%02d  %s\n", hours, minutes, seconds, text_to_write);
    fprintf(fptr, "%02d:%02d:%02d  %s\n", hours, minutes, seconds, text_to_write);
    sem_post(writing);

    fclose(fptr);
}

void malfunctionManager(void)
{
    printf("[%ld] Malfunction Manager process working\n", (long)getpid());
    //while (1)
    //{
    sleep(config->T_Avaria * config->time_units);
    // TODO: aceder a fiabilidade do carro e com base nessa fiabilidade cria uma avaria
    // for (int i = 0; i < team_box_struct)
    // TODO: criar avaria
    //}
    printf("[%ld] Malfunction Manager process finished\n", (long)getpid());
    exit(0);
}

void raceManager(int n_teams)
{
    printf("[%ld] Race Manager process working\n", (long)getpid());

    pid_t teams[n_teams];

    // -------------------- CREATE UNNAMED PIPES -------------------- //

    int channels[n_teams][2];

    // -------------------- OPEN PIPE FOR READING -------------------- //
    int fd;
    if ((fd = open(PIPE_NAME, O_RDONLY)) < 0)
    {
        perror("Cannot open pipe for reading\n");
        exit(1);
    }

    char buffer[LINESIZE];
    int nread;
    nread = read(fd, buffer, LINESIZE);
    buffer[nread - 1] = '\0';
    printf("Received from named pipe: \"%s\"\n", buffer);
    
    close(fd);

    // -------------------- CREATE TEAMS -------------------- //

    for (int i = 0; i < n_teams; i++)
    {
        pid_t teamPID;
        if ((teamPID = fork()) == 0)
        {
            close(channels[i][0]);
            teamManager(channels[i][1], i);
        }
        else if (teamPID == -1)
        {
            write_logfile("ERROR CREATING TEAM MANAGER PROCESS");
            exit(1);
        }
        teams[i] = teamPID; // teams[ID_1, ID_2, ID_3,...]

        // TODO: MANAGE READING OF PIPES
        close(channels[i][1]);
        /* while (1){
            // SELECT PIPES
        }
        */
    }

    // -------------------- WAIT FOR TEAMS -------------------- //

    for (int i = 0; i < n_teams; i++)
    {
        waitpid(teams[i], NULL, 0);
    }

    printf("[%ld] Race Manager process finished\n", (long)getpid());
    exit(0);
}

void teamManager(int channels_write, int teamID)
{
    printf("[%ld] Team Manager #%d process working\n", (long)getpid(), teamID);

    // -------------------- CREATE BOX -------------------- //

    team_box[teamID].box_state = BOX_FREE;

    // TOOO: numCar a ser alterado por named pipe!
    int numCar = 2;

    int carID[config->max_carros];
    pthread_t carThreads[config->max_carros];

    if (numCar > config->max_carros)
    {
        write_logfile("NUMBER OF CARS ABOVE THE MAXIMUM ALLOWED");
        exit(1);
    }

    // -------------------- CREATE CAR THREADS -------------------- //

    for (int i = 0; i < numCar; i++)
    {
        carID[i] = (teamID * numCar) + i;

        // create ids to put as car struct info
        int array_ids[2];
        array_ids[0] = teamID;
        array_ids[1] = carID;

        pthread_create(&carThreads[i], NULL, carThread, array_ids);
        //printf("[%ld] Car #%d thread created\n", (long)getpid(), carID[i]);

        char car_text[LINESIZE];

        sprintf(car_text, "NEW CAR LOADED => TEAM %d, CAR: %02d", teamID, carID[i]);
        write_logfile(car_text);
    }

    // -------------------- MANAGE WRITING PIPES -------------------- //
    // enviar informação do estado do carro para o Race Manager

    /*
    while(1){
        if estado qualquer cena
        write(channels_write,)
    }
    */

    // -------------------- BOX STATE -------------------- //

    //TODO: aceder ao estado da box
    /*
    if
    */

    //TODO: se box livre e carro precisar de ir à box, repara o carro
    //TODO: reparar o carro: sleep de (T_box_min a T_box_max) + sleep de (2 time units)

    // -------------------- WAIT FOR CAR THREADS TO DIE -------------------- //

    for (int i = 0; i < numCar; i++)
    {
        pthread_join(carThreads[i], NULL);
    }

    printf("[%ld] Team Manager #%d process finished\n", (long)getpid(), teamID);
    exit(0);
}

void *carThread(void *array_ids_p)
{
    int *array_ids = (int *)array_ids_p;
    //int carID = *((int *)carID_p);

    // array_ids[0] = TeamID
    // array_ids[1] = carID

    printf("[%ld] Car #%d thread working\n", (long)getpid(), array_ids[1]);

    // -------------------- CREATE CAR STRUCT -------------------- //
    cars[array_ids[1]].id_team = array_ids[0];
    cars[array_ids[1]].combustivel = (float)config->fuel_capacity;
    cars[array_ids[1]].dist_percorrida = 0;
    cars[array_ids[1]].voltas = 0;
    cars[array_ids[1]].state = CORRIDA;

    // -------------------- CAR RACING -------------------- //
    // TODO: receber SPEED do named pipe
    /*while(1){
        sleep(config->time_units * config);

    }*/

    // -------------------- MESSAGE QUEUES -------------------- //
    // TODO: receber mensagem do Malfunction Manager

    // -------------------- ATUALIZA ESTADO -------------------- //
    // TODO: condições estado 1)Corrida 2)Segurança 3)Box 4)Desistência 5)Terminado
    // atualiza consoante mensagem ou combustível
    // se receber mensagem, passa para modo SEGURANÇA

    // -------------------- NOTIFICAR POR UNNAMED PIPE -------------------- //
    // TODO: notificar por UNNAMED PIPE:
    // 1) alteração de estado
    // 2) se terminou a corrida (desistência/ganhou/chegou à meta)

    sleep(5);

    printf("[%ld] Car #%d thread finished\n", (long)getpid(), cars[array_ids[1]]);
    pthread_exit(NULL);
}

int get_line(char * bf, int bfSize){
    if (bfSize < 2) return -1;

    // se nada foi introduzido
    if (fgets(bf, bfSize, stdin) == NULL) return -1;

    /* consome o overflow do buffer
     * caso o \n tenha sido lido pelo fgets, ele estara no maximo na penultima posicao
     * pelo que strcspn apenas devolve buffersize - 1, a ultima posicao, se nao o encontrou
     * o que implica haver ainda input em stdin que ira ser descartado */
    if (strcspn(bf, "\n") == bfSize - 1u) {
        int c = 0;
        while ((c = getchar()) != '\n' && c != EOF && c != 0);
    }

    // substituir \n ou o caracter final por \0
    bf[strcspn(bf, "\n")] = 0;
    return strlen(bf);
}