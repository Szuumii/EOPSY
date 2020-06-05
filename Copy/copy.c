#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

int copy_read_write(int fd_src, int fd_dest);
int copy_nmap(int fd_src, int fd_dest);
void help();

const char** read_paths(int argc, char** argv, int expected);

int main(int argc, char** argv)
{
    bool mem_cpy = false;
    int opt;
    size_t num_of_args;

    while((opt = getopt(argc,argv,":mh")) != -1)
    {
        switch (opt)
        {
        case 'h':
            help();
            return 0;
            break;
        case 'm':
            mem_cpy = true;
            break;
        case '?':
            perror("Unknown parameter");
            exit(1);
        }
    }

    if(argc > 4)
    {
        help();
        return 1;
    }

    if (mem_cpy)
        num_of_args = 4;
    else
        num_of_args = 3;

    const char** files;

    files = read_paths(argc, argv, num_of_args);

    
    int src_file = open(files[1], O_RDONLY);
    if(src_file == -1)
    {
        perror("Error opening src file");
        exit(1);
    }

    struct stat stat_src;
    if(fstat(src_file, &stat_src) == -1)
    {
        perror("Error loading src file information");
        exit(1);
    }

    int dest_file = open(files[2], O_RDWR | O_CREAT, stat_src.st_mode);
    if(dest_file == -1)
    {
        perror("Error opening dest file");
        exit(1);
    }

    int result = mem_cpy ? copy_nmap(src_file,dest_file) : copy_read_write(src_file, dest_file);
    return result;

}


int copy_read_write(int fd_src, int fd_dest)
{
    static const int buffer_size = 4096;
    char buffer[buffer_size];

    int read_file, write_file;

    while((read_file = read(fd_src, buffer,buffer_size)) > 0)
    {
        write_file = write(fd_dest, buffer, read_file);
        if(write_file <= 0)
        {
            perror("Error copy_read_write");
            exit(1);
        }
    }

    return 0;
}


int copy_nmap(int fd_src, int fd_dest)
{
    struct stat stat_src;
    if(fstat(fd_src,&stat_src) == -1)
    {
        perror("Error loading src file mode");
        exit(1);
    }

    char* src_buffer;

    src_buffer = mmap(NULL, stat_src.st_size, PROT_READ,MAP_SHARED,fd_src,0);
    if(src_buffer == (void*)-1)
    {
        perror("Error in mapping memory");
        exit(1);
    }

    if(ftruncate(fd_dest, stat_src.st_size))
    {
        perror("Error changing output file");
        exit(1);
    }

    char* dest_buffer;
    dest_buffer = mmap(NULL, stat_src.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_dest,0);
    if(dest_buffer == (void*)-1)
    {
        perror("Error failed to map memory");
        exit(1);
    }

    dest_buffer = memcpy(dest_buffer, src_buffer, stat_src.st_size);
    if(dest_buffer == (void*)-1)
    {
        perror("Error copying memory");
        exit(1);
    }

    return 0;
}


const char** read_paths(int argc, char** argv, int expected)
{
    if(argc < expected)
    {
        perror("Wrong number of parameters!\n");
        exit(1);
    }

    static const char* retv[2];

    int src_idx = expected - 2;
    int dest_idx = expected - 1;

    retv[1] = argv[src_idx];
    retv[2] = argv[dest_idx];

    return retv;
}


void help()
{
    printf("Usage:\n");
    printf("copy [-m] <file_name> <new_file_name>\n");
    printf("When parameter -m is specified perform copy using nmap method\n");
    printf("Otherwise perform copy using read() and write() method\n");
}