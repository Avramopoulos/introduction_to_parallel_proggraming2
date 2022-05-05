/*****************************************************************************************
  DESCRIPTION:   Συλλογική επικοινωνία programming using MPI (Second asignement in introduction to parallel programming)
  AUTHOR:        George panagiotis Avramopoulos
  CLASS:         ICE1-5006, ice.uniwa.gr 
  DATE:          16/1/2022
  AM:            19390001
*****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <stdbool.h>
#include <limits.h>

bool isDDM(int *receiveArray,int size,int local_num,int my_rank){ //is strictly diagonally dominant == isDDM
	for (int i = 0; i < local_num; i++)
  	 {       
    	    // for each column, finding sum of each row.
     	   int sum = 0;
      	  for (int j = 0; j < size; j++)            
       	     sum += abs(receiveArray[i * size + j]);       
 
       	 // removing the diagonal element.
        	sum -= abs(receiveArray[i * size + my_rank * local_num + i]);
        	
       	 // checking if diagonal element is less
       	 // than sum of non-diagonal element.
       	 if (abs(receiveArray[i * size + my_rank * local_num + i]) < sum)
         	   return false; //asnwer in no 
    	}
      return true ;//answer is yes 
}

int maxD(int *receiveArray,int size,int local_num,int my_rank){
//finds the max of the diagonal values
int max=INT_MIN;
for (int i = 0; i < local_num; i++){          
       	 if(max < abs(receiveArray[i * size + my_rank * local_num + i]))
        		max = abs(receiveArray[i * size + my_rank * local_num + i]);
        	}
return max ;
}


int main(int argc, char** argv)  
{

 struct min_loc{ 
     int   value; 
     int   index; 
 } in, out; 

 int indexi, indexj; 
 int minval; 

 int my_rank,p;
 int i,j,k;
 
 int size,root,local_num,local_max,final_max; 
 bool final_answer,local_answer;
 int *sendArray;
 int **MDBarray;
 int *Barray;
 int finBarray[2500];
 int matrix[50][50];
   
   MPI_Status status;

   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &p);
   
   root=0;
   
   if (my_rank == root)
   {
        printf(" Type the size of the matrix:\n");
	scanf("%d", &size);
	
 	for (i=0; i<size; i++)
 		for (j=0; j<size; j++){
 			printf(" Type the values of 'A' matrix with dimensions %dx%d:\n", size,size);
 			scanf("%d", &matrix[i][j]);
 		}
 		
 	sendArray = (int*)malloc(size * size * sizeof(int));
 	for (i=0; i<size; i++)
 		for (j=0; j<size; j++)
 	sendArray[i * size + j] = matrix[i][j]; //making the 2d array into a continues 1d array so i can scatter it

   }
   
   MPI_Bcast(&size, 1, MPI_INT, root, MPI_COMM_WORLD);
   
   local_num=size/p; //number of lines that each process needs to calculate
   int *receiveArray = (int*)malloc(local_num * size * sizeof(int));
   
   MPI_Scatter(sendArray,local_num * size,MPI_INT,receiveArray,local_num * size,MPI_INT,root,MPI_COMM_WORLD);

   local_answer=isDDM(receiveArray,size,local_num,my_rank); //returns if the local array is strictly diagonally dominant or not
   MPI_Reduce(&local_answer,&final_answer,1,MPI_C_BOOL,MPI_LAND,root,MPI_COMM_WORLD); //logical and for the final answer if the whole matrix is strictly diagonally dominant or not
 
 
   if(my_rank == root)
   	printf("\n Is the matrix strictly diagonally dominant?\n %s \n" ,final_answer ? "YES" : "NO");
   
   MPI_Bcast(&final_answer, 1, MPI_C_BOOL, root, MPI_COMM_WORLD);

   if (final_answer == false ) //ends the the processes and the program if the matrix isn't strictly diagonally dominant
   {
   	MPI_Finalize();
   	return 0;
   }
   
   local_max = maxD(receiveArray,size,local_num,my_rank);
   
   MPI_Reduce(&local_max,&final_max,1,MPI_INT,MPI_MAX,root,MPI_COMM_WORLD); 
   
   if(my_rank == root)
   	printf("\n The max diagonal value is: %d \n" ,final_max);

   MPI_Bcast(&final_max, 1, MPI_INT, root, MPI_COMM_WORLD); //used to calculate the Barray
   //allocating memory for the B array
   Barray=(int*)malloc(local_num * size * sizeof(int));
   //initializing the B array
   for (int i = 0; i < local_num; i++)
   {       
      	for (int j = 0; j < size; j++)            
            Barray[i * size + j] = final_max - abs(receiveArray[i * size + j]);       
        Barray[i * size + my_rank * local_num + i] = final_max;
   }
    	 	
   //finding the local min and loc
   in.value = Barray[0]; 
   in.index = 0; 
   for (int i = 0; i < local_num; i++){       
   	for (int j = 0; j < size; j++){   
   	   	 // comparing the elements to find the min value and its location
   	   	if( in.value > Barray[i * size + j]){
   	   		in.value = Barray[i * size + j];
        		in.index =  j;
        		}
        	}
    	}
   
   in.index = my_rank*size*local_num + in.index; 
   
   MPI_Reduce( &in, &out, 1, MPI_2INT, MPI_MINLOC, root, MPI_COMM_WORLD ); 
   MPI_Gather(Barray,local_num * size,MPI_INT,finBarray,local_num * size ,MPI_INT,root,MPI_COMM_WORLD);
 

   if (my_rank == root)
   {
   	MDBarray = malloc(size * sizeof(int*));
        for(i = 0; i < size; i++)
        {
            MDBarray[i] = malloc(size * sizeof(int));

            if(MDBarray[i] == NULL)
                printf("ERROR: Memory allocation failed!");
        }
 	
 	for (i=0; i<size; i++){
 		for (j=0; j<size; j++){
 			MDBarray[i][j] = finBarray[i * size + j]; //making the 1d array into a 2d
			printf(" \n B array [%d][%d] = %d \n",i,j,MDBarray[i][j]);
		}
	}
	
    minval = out.value; 
    indexi = out.index/size ; 
    indexj = out.index % size; 
    printf("\n The min value is: %d at Barray[%d][%d] \n",minval,indexi,indexj);
   }
    
    MPI_Finalize();
    return 0;
}
