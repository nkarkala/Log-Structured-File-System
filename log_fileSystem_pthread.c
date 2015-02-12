#include"log_fileSystem_pthread.h"


/*
 * The program simulates log structured file system by multithreading using pthread lib
 *
 * 1. Compact thread is blocked until it is required to do compaction( In case of deletion/overriding)
 * 2. The read and write threads proceed with compaction running parallely
 *
   The program gracefully exits by calling exit_procedure that deallocates all the memory incase of invalid input
 *  
 */
pthread_cond_t is_req;
pthread_mutex_t lock;
int compact_needed=0;
/* Shared Resources
 */
IDIR root; // Pointer to root directory
IFILE head; // Pointer to files
LOG linfo; // Log information
IDIR current; // Current directory pointer
/*
 * assign_id -> Assign unique ids to directories and files
 */
int assign_id()
{
   static int num;
   return num+=1;
}
/* 
 * assign_fdes -> Assign the next available file descriptor to the file
 */
int assign_fdes()
{
   static int num[1000];
   int i=0;
   for(i=3; i<1000;i++)
   {
     if(num[i]==0)
     {
       num[i]=1;
        return i;
     }
   }
}
/*
 * find_file_by_path -> Takes absolute path of a file and return a reference to the File if found
 *
 */
IFILE find_file_by_path(char *path)
{
      IFILE cur=head;
      while(cur)
      {
         if(!strcmp(cur->path,path))
         {  
            if(cur->valid==1)
            return cur;
         }
         cur=cur->next;
      }
   return NULL;
}
/*
 *  find_dir_by_path -> Takes absolute path of a dir and return a reference to the Dir if found
 *
 */

IDIR find_dir_by_path(char *path)
{
   IDIR cur=root;
   while(cur)
   {
      if(!strcmp(cur->path,path))
      {
         return cur;
      }
      cur=cur->next;
   }
  return NULL;
}
/*
 *  write_file -> writes a file into memory and prints the information
 *
 */
void *write_file(void *arg)
{
   IFILE new_file=(IFILE)arg;
   IFILE f=NULL;
   char *dup=strdup(new_file->path);
   IFILE cur;

    if(new_file->fdes==0)
    new_file->fdes=assign_fdes(); 
    new_file->id=assign_id();
    new_file->next=NULL;
    new_file->valid=1;
    new_file->start_addr=linfo->last_addr;
    linfo->last_addr=(linfo->last_addr)+(new_file->no_of_blks);
    if(linfo->last_addr >= linfo->max_blks)
    {
       linfo->last_addr=linfo->last_addr-linfo->max_blks;
    }
    linfo->free_blks-=(new_file->no_of_blks);
    if(head == NULL)
    {
       head=new_file;

    } 
    else
    {
      IFILE cur=head;
      while(cur->next!=NULL)
      cur=cur->next;
      cur->next=new_file;
    }
    printf("%s , %d, 0x%05x , %lluKB\n",new_file->path,new_file->fdes,new_file->start_addr,(new_file->no_of_blks)*linfo->blk_size);
 
}
/*
 *  free_file -> Delete a file from memory 
 *  
 */
void free_file(IFILE cur)
{
  if(head==cur)
  {
      head=head->next;
      free(cur);
      return;
  }
  
  IFILE prev=head;
  IFILE temp=prev->next;
  while(temp)
  {
    if(temp==cur)
    {
       prev->next=temp->next;
       free(temp);
       return;
     }
    prev=temp;
    temp=temp->next;
   }
 
}
/*
 *
 *  Compact -> Simulate the compaction of files in a log structured system
 *  
 *  Runs as a separate thread
 */
void *compact(void *arg)
{
  while(1)
  {
    pthread_mutex_lock(&lock); // Mutex to lock compact_needed variable
    while(compact_needed==0)
     pthread_cond_wait(&is_req,&lock); // Wait till compact_needed becomes 1
    pthread_mutex_unlock(&lock);
   /*
      This region is entered after signalling from other threads for the need of compaction
 * */
   IFILE cur=head;
   while(cur)
   { 
     if(cur->valid==0)
        break; 
      cur=cur->next;
   }
   if(cur==NULL)
    continue;
   uint64_t addr=cur->start_addr;
   uint64_t blks=cur->no_of_blks;     
   IFILE temp=head;
   while(temp)
   {  
      if(temp->start_addr < addr)              //  Manipulate address of files
      {
         if(temp->start_addr+ blks < linfo->max_blks)
         temp->start_addr+=blks;
      
      }
      temp=temp->next;
   }
   free_file(cur);
   linfo->free_blks+=blks;
   pthread_mutex_lock(&lock);
   compact_needed=0;       // Lock using mutex before changing compact_needed to 1
   pthread_mutex_unlock(&lock);
 }
 
}
/*
 *  init_dir --> Initializes directory attributes with default values
 */
IDIR init_dir(IDIR dir)
{
   dir->dot_dot=0;
   dir->id=assign_id();
   dir->next=NULL;
   dir->path=NULL;
  return dir;
}
/*
 * make_dir ---> Creates a directory if not already present
 */
void make_dir(char *path)
{
   char *name=strdup(path);
   IDIR parent=find_parent(get_absolute_path(path));
   if(parent==NULL)
   {
     printf("Error : Please check path and try again\n");
     return;
   }
   if(find_dir_by_path(get_absolute_path(path)))
   {
     printf("Error : Directory already exists\n");
     return;
   }

   IDIR new_dir=(IDIR)malloc(sizeof(struct idirectory));
   new_dir=init_dir(new_dir);
   if(!strcmp(parent->path,"/"))
    asprintf(&new_dir->path,"%s%s",parent->path,name);
   else
    asprintf(&new_dir->path,"%s/%s",parent->path,name);
   new_dir->dot_dot=parent->id;
   IDIR cur=root;
   while(cur->next!=NULL)
   cur=cur->next;
   cur->next=new_dir;
   new_dir->next=NULL;
  
}
 /*
    * find_idir_by_id  -> Return reference to a Directory with id given
 */
IDIR find_idir_by_id(int id)
{
   IDIR c=root;
   while(c!=NULL)
   {
        
       if(c->id==id)
        {
           return c;
        }
       c=c->next;
   }
  return NULL;
}
    
/*
 *  change_dir -> Takes a path name and sets current directory to it if it exits
 *
 */
void change_dir(char *path)
{
  IDIR  new_current;
  char *abs_path=get_absolute_path(path);
  new_current=find_dir_by_path(abs_path);
 
  if(new_current!=NULL)
  {
     current=new_current;
  }
  else
  {
    printf("No such directory here\n");
  }

}
/*
 *  find_parent -> Given an absolute path, returns a reference to the parent directory in which it belongs
 *
 */

IDIR find_parent(char *path)
{
   char *dup=strdup(path);
   dirname(dup);
   return(find_dir_by_path(dup));
}
/*
 * delete_file -> Sets the valid flag to file to 0,to indicate its not in use
 * 
 */
void delete_file(char *path)
{
  IFILE result=find_file_by_path(path);
  if(result==NULL)
  {
    printf("Error : No such file!");
  
  }
  else
  result->valid=0;
}
/*
 * get_absolute_path -> Returns absolute path given either Relative/Absolute path
 *
 */
char* get_absolute_path(char *path)
{
   char *saveptr;
   if(path[0]=='/')
   {
      return path;
   }
   else
   {
     char *temp = strtok_r(path,"/",&saveptr);
     char *abs_path;
     if(!strcmp(temp,".."))
     {
        IDIR f=find_idir_by_id(current->dot_dot);
        temp=strtok_r(NULL,"\0",&saveptr);
        if(!strcmp(f->path,"/"))
           asprintf(&abs_path,"%s%s",f->path,temp);
        else
          asprintf(&abs_path,"%s/%s",f->path,temp);
        return abs_path;
      }
      if(!strcmp(current->path,"/"))
         asprintf(&abs_path,"%s%s",current->path,path);
      else
         asprintf(&abs_path,"%s/%s",current->path,path);
      return abs_path;
    }
}
/*
 * init_log -> Initialzes all the log variables taking disk_capacity and block size
 *
 */
LOG init_log(uint64_t disk_capacity,uint64_t blk_size)
{
   uint64_t no_of_blks=disk_capacity/blk_size;
   if(no_of_blks <1 )
   {
     printf("Invalid input\n");
     exit_procedure();
   }
   LOG l=(LOG)malloc(sizeof(struct log));
   l->max_blks=no_of_blks;
   l->free_blks=no_of_blks;
   l->blk_size=blk_size;
   l->last_addr=0;
   l->disk_capacity=disk_capacity;
   return l;
}
/*
 *
 * print_file_info -> reference funtion for printing all the files
 */
void print_file_info()
{
  IFILE f=head;
  if(f==NULL)
  {
     printf("no files to print\n");
     return;
  }
  IFILE cur=f;
  while(cur)
  {
  	printf("Name: %s,fs: %d,addrs: %04x\n",cur->path,cur->fdes,cur->start_addr);
        cur=cur->next;
  }
}
/*
 *
 * extract_function -> Takes the input string and extracts the function/command that needs to be performed
 */
char* extract_function(char *input_str)
{
  
   int i;
   char *output_str;
   char *dup=strdup(input_str);
   output_str=strtok(dup,"(");
   return output_str;
}
/*
 *  extract_parameters -> Takes the whole input command and returns only the parameters under the braces
 */
char* extract_parameters(char *input_str)
{
   int i,j=0;
   char *temp;
   temp=strstr(input_str,"(");
   if(temp[0]!='(' || temp[strlen(temp)-1]!=')')
   {
       exit_procedure();
   }
   temp++;
   temp[strlen(temp)-1]=0;
   return temp;
 
}
/*
 *  convert_to_kilobytes -> Takes input size and converts to KiloBytes unit
 *
 */
uint64_t convert_to_kilobytes(char *input)
{

  uint64_t a;
  char b[100];
  sscanf(input, "%llu%s",&a,b);
  if(!strcmp(b,"MB"))
  {
    return (a*1024);
  }
  else if(!strcmp(b,"GB"))
  {
    return a*1048576;
  }
  else if(!strcmp(b,"TB"))
  {
    return a*1073741824;
  }
  else if(!strcmp(b,"KB"))
  {
     return a;
  }
  else if(!strcmp(b,"B"))
  {
    return a/1024;
  }
  else
  {
    printf("Error : Invalid 3\n");
    exit_procedure();
  }

}   
/*
 *  find_actual_size - Given a file size, calculates the blocks needed and gives the actual file size to be allocated
 *
 */
uint64_t find_actual_size(uint64_t size)
{
   uint64_t blk_size=linfo->blk_size;
   return (size % blk_size) + size;
   
}
/*
 *  read_file -> Reads a file and prints the information
 */
void* read_file(void *arg)
{ 
  char *name=(char*)arg;
  char *abs_path=get_absolute_path(name);
  IFILE result=find_file_by_path(abs_path);
  if(result==NULL)
  {
     printf("Error: Not found!\n");
     return;
  }
  printf("%s, %d, 0x%05x , %lluKB\n",result->path,result->fdes,result->start_addr,(result->no_of_blks)*linfo->blk_size);
  return;

}
/*
 * RemoveSpaces -> Removes all the whitespace characters from the input string
 *
 */
void RemoveSpaces(char * str)
{
  char *write = str, *read = str;
  do {
      if (!isspace(*read))
             *(write++) = *read;
   } while (*(read++));
  
}
/*
 * get_input -> waits for the user command and executes accordingly
 */
void get_input(void)
{
   pthread_t read_id,compact_id,write_id;
   void *exit_status_read;
   void *exit_status_compact;
   void *exit_status_write;

   char input_str[100],inp[100];
   char *in;
   char *func; 
   uint64_t space,blk_size;
   int no_of_char=0;

   do{      
     fgets(input_str, 1000, stdin); 
   }while(sscanf(input_str, "%[^#\n]s\n", input_str)==0);
   RemoveSpaces(input_str);
   func=extract_function(input_str);
   if(!strcmp(func,"diskCapacity"))
   {
      space=convert_to_kilobytes(extract_parameters(input_str));
    
   }  
   else
   {
      printf("Error : No disk Capacity\n");  // If first command is not disk capacity
      exit_procedure();
   }   
   do{
     fgets(input_str, 100, stdin);
   }while(sscanf(input_str, "\n%[^#\n]s\n", input_str)==0);

   RemoveSpaces(input_str);
   func=extract_function(input_str);
   if(!strcmp(func,"blockSize"))
   {
   
    blk_size=convert_to_kilobytes(extract_parameters(input_str));
  
   }
   else
   { 
      printf("Error :  No block size given\n");  // If second command is not block size
      exit_procedure();
   }   
   linfo=init_log(space,blk_size); // Initialze the log
   while(1)
   {
      do{
        fgets(input_str, 1000, stdin);
      }while(sscanf(input_str, "%[^#\n]s", input_str)==0);
     
      RemoveSpaces(input_str);
      func=extract_function(input_str); 
      if(!strcmp(func,"mkdir"))
      {
        char *temp;
        char *dup; 
        char *abs_path;
        char *para= extract_parameters(input_str);
        temp = strtok (para ,",");
        while (temp!= NULL)
        {      
           dup=strdup(temp);
           abs_path=get_absolute_path(temp);
           IDIR parent=find_parent(abs_path);
           if(parent==NULL)
           {
             printf("Error : Please check path and try again\n");
             continue;
           }
           make_dir(temp);
           temp =strtok(NULL, ",");
        } 
      }
      else if(!strcmp(func,"chdir"))
      { 
        char *para=extract_parameters(input_str);
        change_dir(para);    

      }
      else if(!strcmp(func,"write"))
      {
          char *name;
          char *s;
          char *path;
          char *para= extract_parameters(input_str);
          uint64_t size;
          name = strtok (para ,",");
          path= get_absolute_path(name);
          s= strtok(NULL, ",");
          if(!s)
          {
            printf("Error : Wrong number of arguments\n");
            continue;
          }
          if(!strcmp(s,"0"))  // If Size is zero then delete the file(change 'valid status') and signal compact thread  
          {
             delete_file(path);
            /*
               Lock the mutex for variable compact_needed and signal the compact thread
            */
             pthread_mutex_lock(&lock);
             compact_needed=1;
             pthread_mutex_unlock(&lock);
             pthread_cond_signal(&is_req);
          } 
          else
          {
                                    
            size=convert_to_kilobytes(s);
            size=find_actual_size(size);
            int temp_fdes=0,is_overwrite=0;
            IFILE f=find_file_by_path(path);
            if(f!=NULL)
            {
                temp_fdes=f->fdes;
                is_overwrite=1;
                f->valid=0;
            }
            else
            {
                IDIR parent=find_parent(path);
                if(parent==NULL)
                {
                   printf("Invalid path\n");
                   continue;
                }
             }
            double d=(size*1.0)/(linfo->blk_size);
            uint64_t no_of_blks=(uint64_t)ceil(d);
            if(linfo->free_blks < no_of_blks || linfo->free_blks==0)
            {
              if((is_overwrite==0) || ((linfo->free_blks)+f->no_of_blks) < no_of_blks)
              {
                 printf("Error: No sufficient space\n");
                 continue;
              }
             else
             {
                f->valid=0;
               /* if there is enough space to write the file that is getting over written then signal the compact 
                  * thread and proceed
                          */
                pthread_mutex_lock(&lock);
                compact_needed=1;
                pthread_mutex_unlock(&lock);
                pthread_cond_signal(&is_req);
 
             }
                
            }
            IFILE new_file=(IFILE)malloc(sizeof(struct ifile));
            new_file->path=strdup(path);
            new_file->no_of_blks=no_of_blks;
            new_file->fdes=temp_fdes;
            pthread_create(&write_id,NULL,write_file,new_file);   // Write thread
            pthread_join(write_id,&exit_status_write);
          }        
      }
      else if(!strcmp(func,"read"))
      {
         char *name=extract_parameters(input_str);
         pthread_create(&read_id,NULL,read_file,name); // Read thread
         pthread_join(read_id,&exit_status_read);
      }
      else
      {
       
         printf("Error : Invalid 1\n");
         exit_procedure();
      }
   }
}
/*
 * exit_procedure- Exits from the program gracefully by deallocating all the memory and freeing other resources
 */
void exit_procedure()
{
   pthread_mutex_destroy(&lock);
  IFILE cur=head;
  IFILE temp;
 /* Free all the files first */
  while(cur)
  {
     temp=cur; 
     cur=cur->next;
     free(temp);
  }
 /* Free all the directory structures */
  IDIR dcur=root;
  IDIR dtemp;
  while(dcur)
  {
    dtemp=dcur;
    dcur=dcur->next;
    free(dtemp);
  }
  /* Exit from the program */
  exit(0);

}
int main(void)
{
   root=(IDIR)malloc(sizeof(struct idirectory));
   root=init_dir(root);
   root->path="/";
   IDIR last=root;
   IFILE last_file=NULL;
   current=root;
   pthread_t compact_id;
   void *exit_status_compact;

   pthread_mutex_init(&lock,NULL);
   
   pthread_create(&compact_id,NULL,compact,NULL);  // Create compact thread
   get_input();
 
}




