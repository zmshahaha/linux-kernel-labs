#include<unistd.h>
#include<pthread.h>
void *loop()
{while(1);}
int main()
{
	pthread_t t1,t2,t3,t4,t5;
	pthread_create(&t1,NULL,loop,NULL); 	
	pthread_create(&t2,NULL,loop,NULL); 	
	pthread_create(&t3,NULL,loop,NULL); 	
	pthread_create(&t4,NULL,loop,NULL); 	
	pthread_create(&t5,NULL,loop,NULL); 	
	pthread_join(t1,NULL); 	
	pthread_join(t2,NULL); 	
	pthread_join(t3,NULL); 	
	pthread_join(t4,NULL); 	
	pthread_join(t5,NULL); 	
	return 0;
}
