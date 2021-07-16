#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#define MAX_TIME 300
#define TAU 0.5
#define GAMMA 0.2
#define MAX_NODES 10000
#define MAX_EDGES 300
#define INITIAL_INFECTED 1
/*The program takes 30 minutes to run when 10000 and 3000 are used, and even longer when INITIAL_INFECTED is made large*/

/*The following global variables are declared
  -An adjaceny matrix consisting of booleans
  -Arrays keeping a record of the S,I and R values
  -The number of susceptible,infected and recovered people
  -An integer holding the day number
*/
bool adjGraph[MAX_NODES][MAX_NODES];
int S[MAX_TIME],I[MAX_TIME],R[MAX_TIME];
int susceptible,infected,recovered;
int day=1;int heap_size=0;

/*The following structs will be used:
  -graph_node: Contains all the relevant information about the node of the graph
  -node: The node of the priority queue
  -queue: Head of the priority queue
  -data: Linked list node for the neighbours of a graph node
  -list: Head of the above linked list node
*/
typedef struct graph_node
{
	char status;
	int index,pred_inf_time,rec_time;
}graph_node;

typedef struct node
{
	int time,action;//0 action is recover, 1 is transmit
	graph_node u;
}node;

// typedef struct queue
// {
// 	int size;
// 	node *head;
// }queue;

typedef struct data
{
	int index;
	struct data *next;
}data;

typedef struct list
{
	data *head;
}list;

node queue[MAX_NODES*MAX_NODES];

void swap(node *a, node *b) 
{
	node temp = *b;
	*b = *a;
	*a = temp;
}

/*Function to insert into the linked list containing the neigbours of a graph node*/
void insert(list *l, int index)
{
	data *element=(data *)malloc(sizeof(data));
	element->index=index;element->next=NULL;
	if(l->head==NULL) l->head=element;//If list is empty
	else//Insert at end
	{
		for(data *ptr=l->head;ptr!=NULL;ptr=ptr->next)
		{
			if(ptr->next==NULL)
			{
				ptr->next=element;
				break;
			}
		}
	}
}

/*Toss a coin with a probability p of landing heads*/
int coinToss(float p)
{
	int num=10*p;
	int x=rand()%(10);
	if(x<num) return 1;//Heads
	else return 0;//Tails
}

/*Find the minima of 3 numbers*/
int minima(int x,int y,int z)
{
	if(x<y&&x<z) return x;
	else if(y<x&&y<z) return y;
	else return z;
}

/*Generates a random adjaceny matrix*/
void generateMatrix(int V, bool adjMatrix[V][V],list array[V])
{
	for(int i=0;i<V;i++){
		int limit=0;
		//Lines 102 to 116 randomise the graph formation. Line 118 makes the matrix symmetric
		for(int j=i;j<V;j++){
			if(i==0)
			{
				if(!coinToss(0.5)&&limit<MAX_EDGES&&i!=j)
				{
					adjMatrix[i][j]=true;
					limit++;
				}
			}
			else if(!coinToss(0.5)&&limit<MAX_EDGES/2&&i!=j)
			{
				adjMatrix[i][j]=true;
				limit++;
			}
			else adjMatrix[i][j]=false;
			adjMatrix[j][i]=adjMatrix[i][j];
			//If the vertex is a neighbour to another vertex, record it in the neighbour linked list
			if(adjMatrix[i][j])
			{
				insert(&array[i],j);
			}
		}
	}
}

/*The graph nodes are maintained in an array. This function initialises the graph*/
void generateGraph(int V, graph_node Graph[V])
{
	for(int i=0;i<V;i++){
		Graph[i].index=i;
		Graph[i].status='S';
		Graph[i].pred_inf_time=MAX_TIME+69;
		Graph[i].rec_time=0;
	}
}

/*Creates a new event in the priority queue*/
node newNode(int time, int action, graph_node u)
{
	node new;
	new.time=time;new.action=action;new.u=u;
	return new;
}

void heapify(node array[], int heap_size, int i) {
	//printf("here1\n");
	if (heap_size == 1) 
	{
		printf("Single element in the heap");
	} 
	else 
	{
    // Find the largest among root, left child and right child
		int smallest = i;
		int l = 2 * i + 1;
		int r = 2 * i + 2;
		if (l < heap_size && array[l].time < array[smallest].time) smallest = l;
		if (r < heap_size && array[r].time < array[smallest].time) smallest = r;

    // Swap and continue heapifying if root is not largest
		if (smallest!=i) 
		{
			swap(&array[i], &array[smallest]);
			heapify(array, heap_size, smallest);
		}
	}
}

/*Inserts a new event in the priority queue*/
void eventInsert(node array[], int time, int action, graph_node u) 
{
	//printf("here2\n");
	node newEvent=newNode(time,action,u);
	if (heap_size == 0) 
	{
		array[0] = newEvent;
		heap_size += 1;
	} 
	else 
	{
		array[heap_size] = newEvent;
		heap_size += 1;
		for (int i = heap_size / 2 - 1; i >= 0; i--) 
		{
			heapify(array, heap_size, i);
		}
	}
}

/*Delete the front of the queue*/
void eventDelete(node array[], int num) {
	//printf("here3\n");
	int i;
	for (i = 0; i < heap_size; i++) 
	{
		if (num == array[i].time) break;
	}

	swap(&array[i], &array[heap_size - 1]);
	heap_size -= 1;
	for (int i = heap_size / 2 - 1; i >= 0; i--) 
	{
		heapify(array, heap_size, i);
	}
}

/*Extracts front of the queue*/
node extractMin(node array[])
{
	//printf("here4\n");
	node temp=array[0];
	swap(&array[0], &array[heap_size - 1]);
	heap_size -= 1;
	for (int i = heap_size / 2 - 1; i >= 0; i--) {
		heapify(array, heap_size, i);
	}
	return temp;
}


/*Generating the inital number of infected people*/
void initialInfected(int V,graph_node Graph[V])
{
	for(int i=0;i<INITIAL_INFECTED;i++)
	{
		eventInsert(queue,0,1,Graph[i]);
		Graph[i].pred_inf_time=0;
	}
}

/*Recovery of a node*/
void process_rec_SIR(int V,graph_node Graph[V],graph_node u)
{
	//printf("here5\n");
	recovered++;infected--;
	int x=u.index;

	Graph[x].status='R';
	u.status='R';
}

/*Transmit to a node*/
void find_trans_SIR(node array[],int time,graph_node source,graph_node *target)
{
	//printf("here6\n");
	//If the node is susceptible
	if(target->status=='S')
	{
		//Calculate the recovery time
		int inf_time=time;
		do{
			inf_time++;
		}while(!coinToss(TAU));

		//Check if the node can get infected
		if(inf_time<minima(source.rec_time,target->pred_inf_time,MAX_TIME))
		{
			graph_node temp=*target;
			eventInsert(array,inf_time,1,temp);
			target->pred_inf_time=inf_time;
		}
	}
}

/*Transfer the virus to other nodes*/
void process_trans_SIR(int V,graph_node Graph[V],bool adjMatrix[V][V],node arrayQ[],graph_node u,int time,list array[V])
{
	//printf("here7\n");
	susceptible--;infected++;

	int x=u.index;
	//Calculate the recovery time for the source node
	int recovery=time;
	do{
		recovery++;
	}while(!coinToss(GAMMA));
	
	u.rec_time=recovery;
	Graph[x].rec_time=recovery;
	Graph[x].status='I';
	
	//If the node can recover 
	if(Graph[x].rec_time<MAX_TIME)
	{
		graph_node temp=Graph[x];
		eventInsert(arrayQ,recovery,0,temp);
	}

	//Traverse the neighbour linked list
	data *ptr=array[x].head;

	while(ptr)
	{
		int i=ptr->index;
		//Transfer the virus to the neighbour
		find_trans_SIR(arrayQ,time,u,&Graph[i]);
		ptr=ptr->next;
	}
}

/*Simulate the epidemic*/
void fast_SIR(int V,graph_node Graph[V],bool adjMatrix[V][V],list array[V])
{
	susceptible=V;infected=0;recovered=0;
	S[0]=V;I[0]=0;R[0]=0;

	//Create the queue of initially infected people
	initialInfected(V,Graph);

	//While queue is not empty
	while(heap_size>0)
	{
		//Update S,I,R according to the day
		while(day<queue[0].time)
		{
			S[day]=susceptible;
			I[day]=infected;
			R[day]=recovered;
			day++;
		}
		//If the earliest event is a transmit event
		if(queue[0].action==1)
		{
			//If the node is susceptible
			if(queue[0].u.status=='S'&&susceptible>0&&infected<MAX_NODES)
			{
				process_trans_SIR(V,Graph,adjMatrix,queue,queue[0].u,queue[0].time,array);
			}
		}
		//If the event is recovery
		else if(recovered<MAX_NODES&&infected>0)
		{
			process_rec_SIR(V,Graph,queue[0].u);
		}
		extractMin(queue);
	}
}

int main()
{
	srand(time(NULL));
	int V=MAX_NODES;
	//Graph array
	graph_node Graph[V];
	//Array of linked lists(of neighbours)
	list array[V];
	for(int i=0;i<V;i++)
	{
		(array[i].head)=NULL;
	}
	/*Initialise all the required data structures*/
	generateMatrix(V,adjGraph,array);
	printf("\nAdjacency Matrix generated\n");
	generateGraph(V,Graph);
	printf("Graph generated\n\n");

	/*Start the simulation*/
	fast_SIR(V,Graph,adjGraph,array);

	/*The epidemic is considered done when all infected patients have been cured*/
	int i;
    for (i=0;i<MAX_TIME; ++i){
        if(I[i]==0&&I[i+1]==0) break;
    }
    i++;
    //Updating the lists
    if(i>1) S[i-1]=S[i-2];
    else if(i) S[i-1]--;
    R[i-1]=R[i-2]+1;

    /*The epidemic is considered done when all infected patients have been cured*/
    printf("The epidemic is simulated for a population of %d.On day 0, %d person/people get infected\n",MAX_NODES,INITIAL_INFECTED);
    
    for(int j=0;j<i;j++)
    {
        printf("Day %d\n", j);
        printf("S: %d\n", S[j]);
        printf("I: %d\n", I[j]);
        printf("R: %d\n", R[j]);
        printf("\n");
    }
    printf("The epidemic ends after %d days\n", i-1);
    printf("\n");
}