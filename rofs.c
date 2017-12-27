#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

typedef struct ListNode ListNode;
typedef struct LinkedList LinkedList;
typedef struct Information Information;

struct Information {
    char mode[10];
    char code[15];
    char plate_number[5];
    char neighborhood[100];
    char city[50];
    char district[75];
    char latitude[20];
    char longitude[20];
};

struct ListNode {
    ListNode *next;
    char value[100];

    LinkedList *list;
    Information *info;
};

struct LinkedList {
    ListNode *head;
    ListNode *last;
};

//write a functin to find a value in the list
//it will help get rid of repetitive codes
ListNode *find(LinkedList *list, char *to_find) {
    ListNode *current = list->head;

    while(current) {
        if(!strcmp(current->value, to_find)) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

void append(LinkedList *list, char *to_add, Information *info) {
    if(!list->head) {
        list->head = (ListNode *) malloc(sizeof(ListNode));
        memset(list->head, 0, sizeof(ListNode));
        strcpy(list->head->value, to_add);
        list->head->info = info;
        list->last = list->head;

        return;
    }

    ListNode *new_node = (ListNode *) malloc(sizeof(ListNode));
    memset(new_node, 0, sizeof(ListNode));
    strcpy(new_node->value, to_add);
    new_node->info = info;

    list->last->next = new_node;
    list->last = new_node;
}

void delete(LinkedList *list, char *to_remove) {
    if(!list->head) {
        return;
    }

    ListNode *prev = NULL;
    ListNode *current = list->head;

    while(current) {
        if(!strcmp(current->value, to_remove)) {
            if(!prev) {
                if(!list->head->next) {
                    list->head = NULL;
                    list->last = NULL;
                }
                else {
                    list->head = list->head->next;
                }
            }
            else {
                if(current == list->last) {
                    prev->next = NULL;
                    list->last = prev;
                }
                else {
                    prev->next = current->next;
                }
            }
            free(current);
            return;
        }

        prev = current;
        current = current->next;
    }
}

void print_list(LinkedList *list) {
    if(!list) {
        return;
    }

    ListNode *current = list->head;
    while(current) {
        printf("%s\n", current->value);
        print_list(current->list);
        current = current->next;
    }
}

void delete_list(LinkedList *list) {
    if(!list) {
        return;
    }

    ListNode *current = list->head;
    while(current) {
        delete_list(current->list);
        ListNode *temp = current;
        current = current->next;
        free(temp);
    }

    free(list);
}

LinkedList *cities;
LinkedList *districts;
LinkedList *neighborhoods;

LinkedList *plate_numbers;
LinkedList *codes;

Information *entries;
int number_of_entries;

void update_file() {
    FILE *file = fopen("postal-codes.csv", "w");
    char line[250];
    int i = 0;
    for(i = 0; i < number_of_entries; i++) {
        if(strcmp(entries[i].code, "") && strcmp(entries[i].neighborhood, "") && strcmp(entries[i].city, "") && strcmp(entries[i].district, "") && strcmp(entries[i].latitude, "") && strcmp(entries[i].longitude, "")) {
            sprintf(line, "%s\t%s\t%s\t%s\t%s\t%s", entries[i].code, entries[i].neighborhood, entries[i].city, entries[i].district, entries[i].latitude, entries[i].longitude);
            fputs(line, file);    
        }
    }
    fclose(file);
}

Information get_information(const char *path) {
    int len = strlen(path);
    int path_index = 0;
    int buffer_index = 0;
    int number_of_slashes = 0;
    
    char buffer[100];
    memset(buffer, 0, 100);
    
    Information info;
    memset(&info, 0, sizeof(Information));

    for(path_index = 0; path_index <= len; path_index++) {
        if(path[path_index] == '/' || path[path_index] == '\0' || path[path_index] == '.') {
            if(number_of_slashes == 1) {//CODES or NAMES
                strcpy(info.mode, buffer);
            }
            else if(number_of_slashes == 2) {
                if(!strcmp(info.mode, "NAMES")) {//city name
                    strcpy(info.city, buffer);
                }
                else {//plate number
                    strcpy(info.plate_number, buffer);
                }
            }
            else if(number_of_slashes == 3) {
                if(!strcmp(info.mode, "NAMES")) {//district name
                    strcpy(info.district, buffer);
                }
                else {//code
                    strcpy(info.code, buffer);
                }
            }
            else if(number_of_slashes == 4) {
                if(!strcmp(info.mode, "NAMES")) {//neighborhood name
                    strcpy(info.neighborhood, buffer);
                }
            }

            buffer_index = 0;
            memset(buffer, 0, 100);
            number_of_slashes++;
        }
        else {
            buffer[buffer_index++] = path[path_index];
        }
    }

    return info;
}

static int do_getattr( const char *path, struct stat *st ) {
    //printf( "[getattr] Called\n" );
    //printf( "\tAttributes of %s requested\n", path );
    
    st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
    st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
    st->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
    st->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now
    
    Information info = get_information(path);
    int is_directiory = 1;
    if(!strcmp(info.mode, "NAMES")) {
        if(strcmp(info.neighborhood, "")) is_directiory = 0;
    }
    else if(!strcmp(info.mode, "CODES")) {
        if(strcmp(info.code, "")) is_directiory = 0;
    }

    if (is_directiory) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    }
    else {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = 1024;
    }

    return 0;
}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
    //printf( "--> Getting The List of Files of %s\n", path );
        
    filler( buffer, ".", NULL, 0 ); // Current Directory
    filler( buffer, "..", NULL, 0 ); // Parent Directory

    Information info = get_information(path);

    if ( strcmp( path, "/" ) == 0 ) {
        filler( buffer, "NAMES", NULL, 0 );
        filler( buffer, "CODES", NULL, 0 );
    }
    else {
        if(!strcmp(info.mode, "NAMES")) {
            if(!strcmp(info.city, "")) {
                //print all cities
                ListNode *city = cities->head;
                while(city) {
                    filler(buffer, city->value, NULL, 0);
                    city = city->next;
                }
            }
            else {
                if(!strcmp(info.district, "")) {
                    //print all districts in that city
                    ListNode *city = find(cities, info.city);
                    if(city) {
                        ListNode *district = city->list->head;
                        while(district) {
                            filler(buffer, district->value, NULL, 0);
                            district = district->next;
                        }
                    }
                }
                else {
                    if(!strcmp(info.neighborhood, "")) {
                        //print all neighborhoods in that district
                        ListNode *city = find(cities, info.city);
                        if(city) {
                            ListNode *district = find(city->list, info.district);
                            if(district) {
                                ListNode *neighborhood = district->list->head;
                                while(neighborhood) {
                                    char filename[50] = { 0 };
                                    strcat(filename, neighborhood->value);
                                    strcat(filename, ".txt");
                                    filler(buffer, filename, NULL, 0);
                                    neighborhood = neighborhood->next;
                                }
                            }
                        }
                    }
                }
            }
        }
        else if(!strcmp(info.mode, "CODES")) {
            if(!strcmp(info.plate_number, "")) {
                //print all plate numbers
                ListNode *plate_number = plate_numbers->head;
                while(plate_number) {
                    filler(buffer, plate_number->value, NULL, 0);
                    plate_number = plate_number->next;
                }
            }
            else {
                if(!strcmp(info.code, "")) {
                    //print all codes in that plate number
                    ListNode *plate_number = find(plate_numbers, info.plate_number);
                    if(plate_number) {
                        ListNode *code = plate_number->list->head;
                        while(code) {
                            char filename[50] = { 0 };
                            strcat(filename, code->value);
                            strcat(filename, ".txt");
                            filler(buffer, filename, NULL, 0);                                
                            code = code->next;
                        }
                    }
                }
            }
        }
    }

    return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi ) {
    //printf( "--> Trying to read %s, %u, %u\n", path, offset, size );
    
    char message_buffer[250] = { 0 };
    char *selectedText = NULL;
    
    // ... //
    Information info = get_information(path);

    if(!strcmp(info.mode, "NAMES")) {
        ListNode *city = find(cities, info.city);
        if(city) {
            ListNode *district = find(city->list, info.district);
            if(district) {
                ListNode *neighborhood = find(district->list, info.neighborhood);
                if(neighborhood) {
                    sprintf(message_buffer, "code: %s\nneighborhood: %s\ncity: %s\ndistrict: %s\nlatitude: %s\nlongitude: %s", neighborhood->info->code, neighborhood->info->neighborhood, neighborhood->info->city, neighborhood->info->district, neighborhood->info->latitude, neighborhood->info->longitude);
                }   
            }            
        }
    }
    else if(!strcmp(info.mode, "CODES")) {
        ListNode *plate_number = find(plate_numbers, info.plate_number);
        if(plate_number) {
            ListNode *code = find(plate_number->list, info.code);
            if(code) {
                sprintf(message_buffer, "code: %s\nneighborhood: %s\ncity: %s\ndistrict: %s\nlatitude: %s\nlongitude: %s", code->info->code, code->info->neighborhood, code->info->city, code->info->district, code->info->latitude, code->info->longitude);
            }
        }
    }

    selectedText = message_buffer;
    // ... //
    
    memcpy( buffer, selectedText + offset, size );
        
    return strlen( selectedText ) - offset;
}

static int do_rename(const char *old_path, const char *new_path) {
    //printf("renaming from: %s to: %s\n", old_path, new_path);
    //when we are changing the name of the file, should we change the neighborhood value inside that file as well
    Information old_info = get_information(old_path);
    Information new_info = get_information(new_path);

    if(!strcmp(old_info.mode, "NAMES")) {
        ListNode *city = find(cities, old_info.city);
        if(city) {
            ListNode *district = find(city->list, old_info.district);
            if(district) {
                ListNode *neighborhood = find(district->list, old_info.neighborhood);
                if(neighborhood) {
                    strcpy(neighborhood->value, new_info.neighborhood);
                    strcpy(neighborhood->info->neighborhood, new_info.neighborhood);
                }
            }            
        }
    }
    else if(!strcmp(old_info.mode, "CODES")) {
        ListNode *plate_number = find(plate_numbers, old_info.plate_number);
        if(plate_number) {
            ListNode *code = find(plate_number->list, old_info.code);
            if(code) {
                strcpy(code->value, new_info.code);
                strcpy(code->info->code, new_info.code);
            }
        }
    }

    update_file();

    return 0;
}

/** Remove a file */
//we can get rid of the find part sice when we are deleting we are searching the list to find the value again
static int do_unlink(const char *path) {
    //printf("deleting: %s\n", path);
    Information info = get_information(path);
    
    if(!strcmp(info.mode, "NAMES")) {
        ListNode *city = find(cities, info.city);
        if(city) {
            ListNode *district = find(city->list, info.district);
            if(district) {
                ListNode *neighborhood = find(district->list, info.neighborhood);
                if(neighborhood) {
                    memset(neighborhood->info, 0, sizeof(Information));
                    delete(district->list, neighborhood->value);
                }
            }            
        }
    }
    else if(!strcmp(info.mode, "CODES")) {
        ListNode *plate_number = find(plate_numbers, info.plate_number);
        if(plate_number) {
            ListNode *code = find(plate_number->list, info.code);
            if(code) {
                memset(code->info, 0, sizeof(Information));
                delete(plate_number->list, code->value);
            }
        }
    }

    update_file();
    
    return 0;
}

static struct fuse_operations operations = {
    .getattr	= do_getattr,
    .readdir	= do_readdir,
    .read		= do_read,
    .rename     = do_rename,
    .unlink     = do_unlink,
};

int main( int argc, char *argv[] ) {
    number_of_entries = 0;
    FILE *file = fopen("postal-codes.csv", "r");
    char line[250];
    while(fgets(line, 250, file)) {
        number_of_entries++;
    }
    fclose(file);

    file = fopen("postal-codes.csv", "r");
    
    cities = (LinkedList *) malloc(sizeof(LinkedList));
    memset(cities, 0, sizeof(LinkedList));
    districts = (LinkedList *) malloc(sizeof(LinkedList));
    memset(districts, 0, sizeof(LinkedList));
    neighborhoods = (LinkedList *) malloc(sizeof(LinkedList));
    memset(neighborhoods, 0, sizeof(LinkedList));

    plate_numbers = (LinkedList *) malloc(sizeof(LinkedList));
    memset(plate_numbers, 0, sizeof(LinkedList));
    codes = (LinkedList *) malloc(sizeof(LinkedList));
    memset(codes, 0, sizeof(LinkedList));

    entries = (Information *) malloc(number_of_entries * sizeof(Information));

    const char delim[3] = "\t";
    int i = 0;
    for(i = 0; i < number_of_entries; i++) {
        if(fgets(line, 250, file)) {
            strcpy(entries[i].code, strtok(line, delim));
            strcpy(entries[i].neighborhood, strtok(NULL, delim));
            strcpy(entries[i].city, strtok(NULL, delim));
            strcpy(entries[i].district, strtok(NULL, delim));
            strcpy(entries[i].latitude, strtok(NULL, delim));
            strcpy(entries[i].longitude, strtok(NULL, delim));

            if(i) {
                if(!strcmp(entries[i].city, entries[i - 1].city)) {//same city
                    if(!strcmp(entries[i].district, entries[i - 1].district)) {//same district
                        append(neighborhoods, entries[i].neighborhood, &entries[i]);
                        //memcpy(&neighborhoods->last->info, &entries[i], sizeof(Information));
                    }
                    else {//different district
                        districts->last->list = neighborhoods;
                        append(districts, entries[i].district, NULL);

                        neighborhoods = (LinkedList *) malloc(sizeof(LinkedList));
                        memset(neighborhoods, 0, sizeof(LinkedList));
                        append(neighborhoods, entries[i].neighborhood, &entries[i]);
                        //memcpy(&neighborhoods->last->info, &entries[i], sizeof(Information));                        
                    }

                    char code[25];
                    sprintf(code, "%s-%d", entries[i].code, i);
                    append(codes, code, NULL);
                    //memcpy(&codes->last->info, &entries[i], sizeof(Information));                    
                }
                else {//different city
                    districts->last->list = neighborhoods;

                    cities->last->list = districts;
                    append(cities, entries[i].city, NULL);

                    districts = (LinkedList *) malloc(sizeof(LinkedList));
                    memset(districts, 0, sizeof(LinkedList));
                    append(districts, entries[i].district, NULL);

                    neighborhoods = (LinkedList *) malloc(sizeof(LinkedList));
                    memset(neighborhoods, 0, sizeof(LinkedList));
                    append(neighborhoods, entries[i].neighborhood, &entries[i]);
                    //memcpy(&neighborhoods->last->info, &entries[i], sizeof(Information));

                    plate_numbers->last->list = codes;
                    char plate[5];
                    if(strlen(entries[i].code) == 4)
                        sprintf(plate, "%c", entries[i].code[0]);
                    else
                        sprintf(plate, "%c%c", entries[i].code[0], entries[i].code[1]);
                    append(plate_numbers, plate, NULL);

                    codes = (LinkedList *) malloc(sizeof(LinkedList));
                    memset(codes, 0, sizeof(LinkedList));
                    char code[25];
                    sprintf(code, "%s-%d", entries[i].code, i);
                    append(codes, code, NULL);
                    //memcpy(&codes->last->info, &entries[i], sizeof(Information));
                }
            }
            else {
                append(cities, entries[i].city, NULL);
                append(districts, entries[i].district, NULL);
                append(neighborhoods, entries[i].neighborhood, &entries[i]);
                //memcpy(&neighborhoods->last->info, &entries[i], sizeof(Information));
                
                cities->last->list = districts;
                districts->last->list = neighborhoods;
                neighborhoods->last->list = NULL;
                
                char plate[5];
                if(strlen(entries[i].code) == 4)
                    sprintf(plate, "%c", entries[i].code[0]);
                else
                    sprintf(plate, "%c%c", entries[i].code[0], entries[i].code[1]);
                append(plate_numbers, plate, NULL);
                char code[25];
                sprintf(code, "%s-%d", entries[i].code, i);
                append(codes, code, &entries[i]);
                //memcpy(&codes->last->info, &entries[i], sizeof(Information));
                
                plate_numbers->last->list = codes;
                codes->last->list = NULL;
            }

            //memcpy(&entries[i - 1], &entries[i], sizeof(Information));
        }
        else {
            break;
        }
    }
    fclose(file);

    districts->last->list = neighborhoods;
    cities->last->list = districts;

    plate_numbers->last->list = codes;

    //print_list(cities);
    //print_list(plate_numbers);

    int ret = fuse_main( argc, argv, &operations, NULL );

    delete_list(cities);
    delete_list(plate_numbers);

    free(entries);

    //printf("deleted everything\n");

    return ret;
    return 0;
}