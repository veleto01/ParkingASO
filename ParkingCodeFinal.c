#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#define PLAZAS_MAX 6
#define N_CAM 2

int plazasParking[PLAZAS_MAX];
int plazasLibres = PLAZAS_MAX;
int nprocs;

///////////////////////////////////////////////////
//FUNCIONES AUXILIARES (DEFINICION = DECLARACION)//
///////////////////////////////////////////////////
void park(int vehiculo){
    //CASO 0: Es un coche
    if (vehiculo <= nprocs - 1 - N_CAM){

        //Aparcamos el coche
        for (int i = 0; i < PLAZAS_MAX; i++){
			
            if (plazasParking[i] == -1){
				
                //Encontramos la plaza donde tiene que ir el coche
                plazasParking[i] = vehiculo;
                plazasLibres--;
                printf("Coche numero %d aparcando en plaza numero %d; quedan %d plazas libres\n", vehiculo, i, plazasLibres);
                printParking();
                MPI_Send(&i, 1, MPI_INT, vehiculo, 0, MPI_COMM_WORLD);
                break;
				
            }
        }
    }
    //CASO 1: Es un camion
    else{
        
		for (int i = 0; i < PLAZAS_MAX; i++){
            
			if (plazasParking[i] == -1 && i + 1 < PLAZAS_MAX && plazasParking[i + 1] == -1){
                
				//Encontramos la plaza donde tiene que ir el coche
                plazasParking[i] = vehiculo;
                plazasParking[i + 1] = vehiculo;
                plazasLibres -= 2;
                printf("Camion numero %d aparcando en plazas numero %d y %d; quedan %d plazas libres\n", vehiculo, i, i + 1, plazasLibres);
                printParking();
                MPI_Send(&i, 1, MPI_INT, vehiculo, 0, MPI_COMM_WORLD);
                break;
            }
        }
    }
}

void quitarEsperando(int *vehiculosEsperando){
    for (int i = 0; i < nprocs - 1; i++){
		
        if (i == nprocs - 2) vehiculosEsperando[i] = -1;
        
        else vehiculosEsperando[i] = vehiculosEsperando[i + 1];
		
    }
}

int hayPlazasContiguas(){
    int hayPlazasCont = 0;
    for (int i = 0; i < PLAZAS_MAX; i++){
		
        if (i + 1 < PLAZAS_MAX && plazasParking[i] == -1 && plazasParking[i + 1] == -1){
			
            hayPlazasCont = 1;
            break;
        }
    }
	
    return hayPlazasCont;
}

int* vehiculosEnEspera(int *cola, int vehiculo){
    for (int i = 0; i < nprocs - 1; i++){
        
		if (cola[i] == -1){
			
            cola[i] = vehiculo;
            break;
        }
    }
    
    return cola;
}

void printParking(){
    printf("Parking: [");
	
    for (int i = 0; i < PLAZAS_MAX; i++) printf("%d, ", plazasParking[i]);
	
    printf("]\n");
}

void printCola(int *cola){
    printf("Cola esperando: [");
	
    for (int i = 0; i < nprocs - 1; i++) printf("%d, ", cola[i]);
    
    printf("]\n");
}

////////
//MAIN//
////////
int main(int argc, char **argv){
    int rank, recibe;
	
	//Inicialización de los procesos MPI
    MPI_Init(&argc, &argv);
    MPI_Status estado;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	//Inicialización de planta 
    for (int i = 0; i < PLAZAS_MAX; i++) plazasParking[i] = -1;

    if (rank == 0){
		
        int *vehiculosEsperando = (int *)malloc((nprocs - 1) * sizeof(int));
		
        for (int i = 0; i < nprocs - 1; i++) vehiculosEsperando[i] = -1;

        while (rank == 0){

            MPI_Recv(&recibe, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &estado);
			
            //0 = no aparcado	//	1 = Aparcado
            int isAparcado = 0;

            for (int i = 0; i < PLAZAS_MAX; i++){
				
                if (plazasParking[i] == recibe){
					
                    isAparcado = 1;	//El coche esta aparcado
                    break;
                }
            }
			
            //Quiere aparcar ->
            if (isAparcado == 0){
				
                //Sabemos que es un coche cuando "coche<=nprocs-1-N_CAM"
                if (plazasLibres > 0 && vehiculosEsperando[0] == -1 && recibe <= nprocs - 1 - N_CAM){
					
                    park(recibe);
                }
				
                //Sabemos que es un camion cuando "camion>nprocs-1-N_CAM"
                else if (plazasLibres >= 2 && vehiculosEsperando[0] == -1 && recibe > nprocs - 1 - N_CAM){
					
                    int hayPlazasCont = hayPlazasContiguas();

                    if (hayPlazasCont == 1) park(recibe);

					//El camion no se encuentra en la cola, por lo que debemos añadirlo
                    else{
						
                        vehiculosEsperando = vehiculosEnEspera(vehiculosEsperando, recibe);
                        printf("Camión %d esperando\n", recibe);
                        printCola(vehiculosEsperando);
                    }
					
                }
                else if (plazasLibres > 0 && vehiculosEsperando[0] != -1){
					
                    //Ponemos en cola al coche que iba a entrar nuevo
                    vehiculosEsperando = vehiculosEnEspera(vehiculosEsperando, recibe);
                    
                    if(recibe <= nprocs - 1 - N_CAM) printf("Coche %d esperando\n", recibe);
                    
                    if(recibe > nprocs - 1 - N_CAM) printf("Camion %d esperando\n", recibe);
                    
                    printCola(vehiculosEsperando);
                    
					//Si es un coche
                    if (vehiculosEsperando[0] <= nprocs - 1 - N_CAM){

                        park(vehiculosEsperando[0]);
                        //Eliminamos el coche de la cola de "vehiculosEsperando"
                        quitarEsperando(vehiculosEsperando);
                    }
					
                    //Si es un camion
                    else if (vehiculosEsperando[0] > nprocs - 1 - N_CAM && plazasLibres >= 2){
						
                        int hayPlazasCont = hayPlazasContiguas();

                        if (hayPlazasCont == 1){
							
                            park(vehiculosEsperando[0]);
							
                            //Eliminamos el camion de la cola de "vehiculosEsperando"
                            quitarEsperando(vehiculosEsperando);
                        }
                    }
                }
				
                else{
					
                    vehiculosEsperando = vehiculosEnEspera(vehiculosEsperando, recibe);
			
					if(recibe <= nprocs - 1 - N_CAM)	printf("Coche %d esperando\n", recibe);
                    
                    if(recibe > nprocs - 1 - N_CAM)		printf("Camion %d esperando\n", recibe);
                    
                    printCola(vehiculosEsperando);
                }
            }
			
            //Quiere salir
            else{
				
                //Si es un coche
                if (recibe <= nprocs - 1 - N_CAM){
					
                    for (int i = 0; i < PLAZAS_MAX; i++){
						
                        if (plazasParking[i] == recibe){
							
                            //Sale el coche
                            plazasParking[i] = -1;
                            plazasLibres++;
                            printf("Saliendo coche %d de la plaza %d, quedan %d plazas libres\n", recibe, i, plazasLibres);
                            printParking();
							
                            //Si el siguiente en cola es un coche
                            if (vehiculosEsperando[0] != -1 && plazasLibres > 0 && vehiculosEsperando[0] <= nprocs - 1 - N_CAM){
								
                                park(vehiculosEsperando[0]);
                                //Eliminamos el coche de la cola de "vehiculosEsperando"
                                quitarEsperando(vehiculosEsperando);
                            }
							
                            //Si el siguiente en cola es un camion
                            else if (vehiculosEsperando[0] != -1 && plazasLibres >= 2 && vehiculosEsperando[0] > nprocs - 1 - N_CAM){
								
                                //0 = No contiguas //	1 = Contiguas
                                int hayPlazasCont = hayPlazasContiguas();

                                if (hayPlazasCont == 1){
									
                                    park(vehiculosEsperando[0]);
                                    //Eliminamos el camion de la cola de "vehiculosEsperando"
                                    quitarEsperando(vehiculosEsperando);
                                }
                            }
                            break;
                        }
                    }
                }
				
                //si es un camion
                else{
					
                    for (int i = 0; i < PLAZAS_MAX; i++){
						
                        if (plazasParking[i] == recibe){
							
                            //Sale el camion
                            plazasParking[i] = -1;
                            plazasParking[i + 1] = -1;
                            plazasLibres += 2;
                            printf("Saliendo camion %d de la plaza %d, quedan %d plazas libres\n", recibe, i, plazasLibres);
                            printParking();
							
                            //Si es un coche
                            while (vehiculosEsperando[0] != -1 && plazasLibres > 0 && vehiculosEsperando[0] <= nprocs - 1 - N_CAM){
								
                                park(vehiculosEsperando[0]);
                                //Eliminamos el camion de la cola de "vehiculosEsperando"
                                quitarEsperando(vehiculosEsperando);
                            }
							
                            //Si es un camion
                            while (vehiculosEsperando[0] != -1 && plazasLibres >= 2 && vehiculosEsperando[0] > nprocs - 1 - N_CAM){
								
                                int hayPlazasCont = hayPlazasContiguas();

                                if (hayPlazasCont == 1){
									
                                    park(vehiculosEsperando[0]);
                                    //Eliminamos el coche de la cola de "vehiculosEsperando"
                                    quitarEsperando(vehiculosEsperando);
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    else{
		
        while (rank != 0){
			
			//Gestion de los comunicadores MPI
            MPI_Send(&rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
			
            int plaza;
			
            MPI_Recv(&plaza, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &estado);
			
            usleep((rand() % 500 + 300));
			
            MPI_Send(&rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
			
            usleep((rand() % 1000 + 300));
        }
    }
	
    MPI_Finalize();
    return 0;
}

////////////////////////////////////////
//Autores:      					  // 	
//- Mario García Barreno			  //
//- Manuel Alejandro Jiménez Fernández//
//- Óscar Utrilla Mora                //
////////////////////////////////////////
