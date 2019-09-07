#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  TODO:
  - fixa il report.
*/

#define ENTITY_TABLE_SIZE  312353 // 312353, 711353, 53549, 6451, 27919
#define SUB_RELATIONS_ARRAY_SIZE_TOT 147456 // 3 * 49152
#define RELATIONS_BUFFER_SIZE 128 //4096
//#define COLLISION_BUFFER_SIZE 16 // *16 = <<4

/* add -Ddeb to gcc compiler options to compile in verbose debug mode */

//type definitions--------------------------------------------------------------

typedef char SuperLongString[5632];
typedef char LongString[128];
typedef char String[32];

typedef struct {
  String name;
  unsigned int value;
  int rel_num;
} Entity;

typedef struct {
  String name;
  int sub_relation_number;
  void* subrel_array[SUB_RELATIONS_ARRAY_SIZE_TOT];
  // +0 SOURCES | +1 DESTINATIONS | +2 DEST_NAMES
} Relation;

int number_of_relations = 0;
#ifdef deb
  int collisions = 0; //for testing the hash function
#endif

//------------------------------------------------------------------------------
//functions definitions---------------------------------------------------------

unsigned long entity_get_hash_old(char* name)
{
  unsigned int hash = 0;

  for (int i=1; name[i]!='"'; i++)
    hash = name[i] + (hash << 6) + (hash << 16) - hash;

  #ifdef deb
    printf("entity hash: %lu\n", hash);
  #endif

  return hash;
}
//SDBM hashing algorithm

inline __attribute__((always_inline)) unsigned int entity_get_hash(char* name)
{
  int i;
  unsigned int byte, crc, mask;

   i = 1;
   crc = 0xFFFFFFFF;
   while (name[i] != '"')
   {
      byte = name[i];            // Get next byte.
      crc = crc ^ byte;
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
      ++ i;
   }
   return ~crc;
}
//CRC32b algorithm

int get_argument(LongString input_string, char* dest_string, int start_pos)
{
  input_string += start_pos;
  dest_string[0] = '"';
  int string_length = 1;
  while (*input_string != '"') //while the char is valid
    dest_string[string_length++] = *input_string++;
  dest_string[string_length] = '"';
  dest_string[string_length+1] = '\0';
  return start_pos+string_length;
}
//given a string in stdin, it fills dest_string with the input_string
//until a ' " ' is found. Returns position in input string.

inline __attribute__((always_inline)) Entity* get_entity(Entity** entity_table, String name)
{
  #ifdef deb
    printf("getting entity %s.\n", name);
  #endif
  int pos = (entity_get_hash(name) % ENTITY_TABLE_SIZE);
  if (entity_table[pos] != NULL)
    return entity_table[pos];
  return 0;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

int relations_binary_search(Relation* relations_buffer[RELATIONS_BUFFER_SIZE],
String name, int l, int r)
{
  while (r >= l)
  {
        #ifdef deb
          printf("binary search %d, %d\n", l, r);
        #endif
        int mid = l + (r - l) / 2;
        int val = strcmp(name, relations_buffer[mid]->name);
        if (val == 0)
        {
            #ifdef deb
              printf("found relation %s.\n", name);
            #endif
            return mid;
        }
        if (val < 0)
            r = mid - 1;
        else
            l = mid + 1;
  }
  #ifdef deb
    printf("no match found for %s.\n", name);
  #endif
  return -1;
}

int sub_relations_binary_search(Entity** dest_array, Entity** src_array, String** dna,
Entity* dest, int l, int r, Entity* src, char* dest_name)
{
  int mid, pos;
  while (r >= l)
  {
        #ifdef deb
          printf("sub rel binary search %d, %d\n", l, r);
        #endif
        mid = l + ((r - l) >> 1);
        pos = mid*3;
        if (dest_array[pos] == dest)
        {
            #ifdef deb
              printf("checking entity %s.\n", (*dna[pos]));
            #endif
            Entity* val = src_array[pos];
            if (val == src)
            {
              return mid;
            }
            if (val > src)
                r = mid - 1;
            else
                l = mid + 1;
        }
        else
        {
          #ifdef deb
            printf("nope.\n");
          #endif
          if (strcmp(*dna[pos], dest->name) > 0)
            r = mid - 1;
          else
            l = mid + 1;
        }
  }
  #ifdef deb
    printf("r(%d) < l(%d)\n", r, l);
  #endif
  return -1;
}
//looks for a sub relation, return it's position if it exists,
//else return -1

int sub_relations_binary_search_creation(Entity** dest_array,
Entity** src_array, String** dna, Entity* dest, int l, int r, Entity* src, char* dest_name)
{
  int mid, pos;
  while (r >= l)
  {
        #ifdef deb
          printf("binary search %d, %d\n", l, r);
        #endif
        mid = l + ((r - l) >> 1);
        pos = mid*3;
        if (dest_array[pos] == dest)
        {
            Entity* val = src_array[pos];
            if (val == src)
            {
              #ifdef deb
                printf("relation already exists.\n");
              #endif
              return -1;
            }
            if (val < src)
              l = mid + 1;
            else
              r = mid - 1;
        }
        else if (strcmp(*dna[pos], dest_name) > 0)
        {
            #ifdef deb
              printf("%s < %s\n", dest_name, dest_array[pos]->name);
            #endif
            r = mid - 1;
        }
        else
        {
          #ifdef deb
            printf("%s > %s\n", dest->name, dest_array[pos]->name);
          #endif
          l = mid + 1;
        }
  }
  #ifdef deb
    printf("r(%d) < l(%d)\n", r, l);
  #endif
  return l;
}
//looks for a sub relation, if it does not exist, return the position in which
//to create the new relation, else return -1

Relation* get_relation(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name)
{
  #ifdef deb
    printf("getting relation %s.\n", name);
  #endif
  int rel_num = number_of_relations;

  if (rel_num == 0)
    return 0;

  int val = relations_binary_search(relations_buffer, name, 0, rel_num-1);
  if (val != -1)
    return relations_buffer[val];
  else
    return 0;
}
//returns the pointer to the wanted relation if it exists, else returns 0

inline __attribute__((always_inline)) Entity* create_entity(String name, unsigned int value)
{
  //create the entity in the hash table
  Entity* self;
  self = (Entity*) malloc(sizeof(Entity));
  strcpy(self->name, name);
  self->value = value;
  self->rel_num = 0;
  return self;
}
//create entity, return the pointer to the created entity.

inline __attribute__((always_inline)) void rel_buffer_fixup(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], int start_pos, Relation* replacer, int end)
{
  Relation** dest = relations_buffer+(start_pos);
  memmove( (void*) (dest+1), (void*) dest, (end-start_pos) << 3);
  *((Relation**) dest) = replacer;
}
//fixup the relation buffer after adding a relation.

inline __attribute__((always_inline)) void rel_buffer_fixup_delete(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], int start_pos, const int end)
{
  Relation** dest = relations_buffer+(start_pos);
  memmove( (void*) dest, (void*) (dest+1), (end-start_pos) << 3);
}
//fixup the relation buffer after deleting a relation.

inline __attribute__((always_inline)) void sub_rel_array_fixup_complete(
void* subrel_array, const int start_pos, const int end, Entity* dest_replacer,
Entity* src_replacer, String* dest_name_replacer)
{
  Entity** array = (Entity**) subrel_array+start_pos*3;
  #ifdef deb
    printf("doing the addition fixup (%d)...\n", ((end-start_pos)));
    printf("%s, %s, %s\n", ((*((Entity**) subrel_array))->name),
    (*((Entity**) subrel_array+1))->name, *(((String**) subrel_array)+2));
  #endif
  memmove( (void*) (array+3), (void*) array, ((end-start_pos)*24));
  *( (Entity**) array) = dest_replacer;
  *( (Entity**) array+1) = src_replacer;
  *( (String**) array+2) = dest_name_replacer;
}
//given the pointers to the arrays to restore, the starting position, the
//end postion and the replacers, this function restores
//the order in the arrays.

inline __attribute__((always_inline)) void sub_rel_array_fixup_delete_complete(
void* subrel_array, int start_pos, const int end)
{
  Entity** array = (Entity**) subrel_array+start_pos*3;
  #ifdef deb
    printf("doing the deletion fixup...\n");
  printf("%s, %s, %s\n", ((*((Entity**) subrel_array))->name),
  (*((Entity**) subrel_array+1))->name, *(((String**) subrel_array)+2));
  #endif
  memmove( (array), (array+3), (end-start_pos) << 3);
}
//given the pointers to the arrays to restore, the starting position, the
//end postion and the replacers, this function restores
//the order in the arrays.

inline __attribute__((always_inline)) void sub_rel_array_fixup_delete_complete_static(
void* subrel_array, int start_pos, const int end, int step)
{
  Entity** array = (Entity**) subrel_array+start_pos*3;
  #ifdef deb
    printf("doing the static deletion fixup...\n");
    printf("%s, %s, %s\n", ((*((Entity**) subrel_array))->name),
    (*((Entity**) subrel_array+1))->name, *(((String**) subrel_array)+2));
  #endif
  memmove( (array), (array+step), (end-start_pos) << 3);
}
//given the pointers to the arrays to restore, the starting position, the
//end postion and the replacers, this function restores
//the order in the arrays.

Relation* create_relation(String name)
{
  //create the relation to put in hash table
  Relation* self = (Relation*) malloc(sizeof(Relation));
  memcpy(self->name, name, sizeof(String));
  self->sub_relation_number = 0;
  return self;
}
//Create relation in hash table, create relation in ordered list and
//return the pointer to the relation in hash table.

int relation_add_entities(Relation* rel, Entity* src, Entity* dest, String dest_name)
{
  //setup
  int rel_num = rel->sub_relation_number;

  Entity** dest_array = (Entity**) rel->subrel_array;
  Entity** src_array = dest_array+1;
  String** dest_name_array = (String**) dest_array+2;

  if (rel_num == 0)
  {
    #ifdef deb
      printf("created sub relation in first slot of relation\n");
    #endif
    dest_array[0] = dest;
    src_array[0] = src;
    rel->sub_relation_number ++;
    src->rel_num ++;
    dest->rel_num ++;
    dest_name_array[0] = &(dest->name);
    return 1;
  }
  else
  {
    int mid_val = sub_relations_binary_search_creation(dest_array, src_array, dest_name_array, dest, 0, rel_num-1, src, dest_name);
    if (mid_val > -1)
    {
      #ifdef deb
        printf("found a spot\n");
      #endif
      rel->sub_relation_number ++;
      src->rel_num ++;
      dest->rel_num ++;
      if (mid_val < rel_num)
      {
        #ifdef deb
          printf("adding sub relation in %d.............\n", mid_val);
        #endif
        sub_rel_array_fixup_complete(dest_array, mid_val, rel_num, dest, src, &(dest->name));
      }
      else
      {
        #ifdef deb
          printf("adding sub relation at the end of the buffer...........\n");
        #endif
        mid_val *= 3;
        dest_array[mid_val] = dest;
        src_array[mid_val] = src;
        dest_name_array[mid_val] = &(dest->name);
      }
      return 1;
    }
    return 0;
  }
}
//adds entities to the new/existing relation;
//return 0 if everything went okay, else 1.

void entity_array_fixup_delete(Entity** array, int start)
{
  int i;
  for (i=start; array[i+1] != NULL; i++)
    array[i] = array[i+1];
  array[i] = NULL;
}
//fixup the entity array after deleting one.

int handle_entity_creation(Entity** entity_table, String name)
{
  //do the hash function but also save the value of the entity
  unsigned int value = entity_get_hash(name);
  int pos = (value % ENTITY_TABLE_SIZE);
  #ifdef deb
    printf("entity position: %d\nhash: %u\n", pos, value);
  #endif

  if (entity_table[pos] == NULL)
  {
    entity_table[pos] = create_entity(name, value);
    return 1;
  }
  #ifdef deb
    printf("entity already exists.\n");
  #endif
  return 0;
}
//handle entity creation, return the created entity if success, else 0

int handle_relation_creation(Entity** entity_table, Relation** relations_buffer,
String name1, String name2, String name, int sig_forget)
{
  Entity* e1 = get_entity(entity_table, name1);
  if (e1 == 0)
  {
    #ifdef deb
      printf("source does not exist\n");
    #endif
    return 0;
  }
  Entity* e2 = get_entity(entity_table, name2);
  if (e2 == 0)
  {
    #ifdef deb
      printf("destination does not exist\n");
    #endif
    return 0;
  }

  static String prev_relation_name = "";
  static Relation* prev_rel = 0;

  #ifdef deb
    printf("all of the entities exist\nPREVREL: %s\n", prev_relation_name);
  #endif

  if (sig_forget == 0)
  {
    if (strcmp(prev_relation_name, name) == 0)
      return relation_add_entities(prev_rel, e1, e2, name2);
  }
  else
  {
    #ifdef deb
      printf("forgot previous relation...\n");
    #endif
    prev_relation_name[0] = '\0';
  }

  int rel_num = number_of_relations;

  if (rel_num == 0)
  {
    relations_buffer[0] = create_relation(name);
    number_of_relations ++;
    return relation_add_entities(relations_buffer[0], e1, e2, name2);
  }

  int r = rel_num-1, l = 0, val, mid;

  while (r >= l)
  {
        #ifdef deb
          printf("binary search %d, %d\n", l, r);
        #endif
        mid = l + ((r - l) >> 1);
        val = strcmp(name, relations_buffer[mid]->name);
        if (val == 0)
        {
            #ifdef deb
              printf("found relation %s.\n", name);
            #endif
            prev_rel = relations_buffer[mid];
            memcpy(prev_relation_name, name, sizeof(String));
            return relation_add_entities(relations_buffer[mid], e1, e2, name2);
        }
        if (val < 0)
            r = mid - 1;
        else
            l = mid + 1;
  }

  #ifdef deb
    printf("created new relation in %d\n", l);
  #endif

  Relation* self = create_relation(name);
  prev_rel = self;
  memcpy(prev_relation_name, name, sizeof(String));
  rel_buffer_fixup(relations_buffer, l, self, number_of_relations);
  number_of_relations ++;
  return relation_add_entities(self, e1, e2, name2);
}
//handle relation creation, return 0 if nothing was created, the relation
//created or modified otherwise.

Entity* delent_function(Entity** entity_table, String name)
{
  unsigned int value = entity_get_hash(name);
  int pos = (value % ENTITY_TABLE_SIZE);
  Entity* entity = entity_table[pos];

  if (entity == NULL)
  {
    #ifdef deb
      printf("entity does not exist entity.\n");
    #endif
    return 0;
  }

  entity_table[pos] = NULL;

  #ifdef deb
    printf("deallocated entity at %p, relations = %d\n", (void*) entity, entity->rel_num);
  #endif
  if (entity->rel_num != 0)
    return entity;
  return 0;
}
//deallocates an entity; return the entity pointer if succes, else 0

void delete_unused_relation(Relation** relations_buffer, Relation* rel, int start_pos, int num)
{
  rel_buffer_fixup_delete(relations_buffer, start_pos, num);
  free(rel);
}
//deletes a relation with no sub relations.

int delrel_function(Entity** entity_table, Relation** relations_buffer,
String name_source, String name_dest, String rel_name, int sig_forget)
{
  //static variables for speedy checking
  static Relation* prev_rel = 0;
  static String prev_rel_name = "";

  #ifdef deb
    printf("PREVREL: %s\n", prev_rel_name);
  #endif

  //get relation pointer
  Relation* rel;
  if (sig_forget == 0)
  {
    if (strcmp(prev_rel_name, rel_name) == 0)
    {
      #ifdef deb
        printf("same as previous relation (%s)\n", prev_rel_name);
      #endif
      if (prev_rel->sub_relation_number == 0)
      {
        #ifdef deb
          printf("relation %s is empty, exiting...\n", prev_rel_name);
        #endif
        prev_rel_name[0] = '\0';
        return 0;
      }
      rel = prev_rel;
    }
  }
  else
  {
    #ifdef deb
      printf("forgot previous relation...\n");
    #endif
    prev_rel_name[0] = '\0';
  }

  rel = get_relation(relations_buffer, rel_name);
  if (rel == 0)
  {
    #ifdef deb
      printf("relation does not exist.\n");
    #endif
    return 0;
  }
  prev_rel = rel;
  memcpy(prev_rel_name, rel_name, sizeof(String));

  Entity* src = get_entity(entity_table, name_source);
  if (src == 0)
  {
    #ifdef deb
      printf("source does not exist\n");
    #endif
    return 0;
  }
  Entity* dest  = get_entity(entity_table, name_dest);
  if (dest == 0)
  {
    #ifdef deb
      printf("source does not exist\n");
    #endif
    return 0;
  }

  #ifdef deb
    printf("all of the entities exist\n");
  #endif

  int i;
  //delete sub relation from relation list in relation
  #ifdef deb
    printf("relation may exist.\n");
  #endif

  Entity** dest_array = (Entity**) rel->subrel_array;
  Entity** src_array = dest_array+1;
  String** dest_name_array = (String**) dest_array+2;

  i = sub_relations_binary_search(dest_array, src_array, dest_name_array, dest, 0, rel->sub_relation_number-1, src, name_dest);
  if (i > -1)
  {
      int rr = rel->sub_relation_number;
      rel->sub_relation_number --;
      src->rel_num --;
      dest->rel_num --;
      sub_rel_array_fixup_delete_complete(dest_array, i, rr);
      #ifdef deb
        printf("deleted relation at i = %d\n", i);
      #endif
      return 1;
  }
  #ifdef deb
    printf("relation does not exist (LATE)\n");
  #endif
  return 0;
}
//deletes a relation; return 1 if success, else 0

int check_entity_validity(Entity* entity, Entity** array, int del_num)
{
  int i = del_num;
  while (i != 0)
    if (entity == array[--i])
      return 0;
  return 1;
}
//checks if the given entity hasn't been deleted.

char* fast_strcat(char* dest, char* src)
{
  while (*dest) dest++;
  while (*dest++ = *src++);
  return --dest;
}
//a version of strcat that returns a pointer to the last char of the string.

int report_function( Relation* relations_buffer[RELATIONS_BUFFER_SIZE], int update, Entity** del_array, int del_num)
{
  //if there are no relations, do nothing
  if (number_of_relations == 0)
  {
    puts("none");
    return 0;
  }

  static SuperLongString report_string;
  int sig_forget = 0;

  if (update == 1) //if the report needs to be updated:
  {
    #ifdef deb
      printf("something changed in the relations, updating report string...\n");
    #endif
    report_string[0] = '\0';
    char* p = report_string;
    //aux variables
    Entity* prev_entity;
    Entity* temp_entity;
    Entity** dest_array;
    Entity** src_array;
    String** dest_name_array;
    Relation* relation;
    char tmp[20];
    int k, temp_counter = 0;
    int maximum;
    int rel_num;
    int rr = number_of_relations;
    //report data structures
    String* rel_report_temp[128];
    int rel_num_temp;

    if (del_num > 0) //"clean" the array before doing the report.
    {

      int mode;
      //0 = normal, no mode selected
      //1 = burst delete, found an invalid destination, delete all consecutive sub relations with the same destination.
      //2 = valid streak,found a valid destination, only check the source of the consecutive subrelations.

      #ifdef deb
        printf("entities have been deleted, cleaning up...\n");
      #endif

      int h = 0;
      while (h < rr)
      {
        relation = relations_buffer[h];
        rel_num = (relation->sub_relation_number)*3;
        dest_array = (Entity**) relation->subrel_array;
        src_array = dest_array+1;
        dest_name_array = (String**) dest_array+2;
        mode = 0;

        if (rel_num > 0)
        {
          int i = 0;
          int temp_start;
          prev_entity = NULL;
          temp_entity = dest_array[0];

          temp_counter = 0;
          rel_num_temp = 0;
          maximum = -1;

          while (i<rel_num)
          {
            switch (mode)
            {
              case 0: //normal mode
              {
                #ifdef deb
                  printf("mode 0:\n");
                #endif
                if (check_entity_validity(temp_entity, del_array, del_num))
                {
                  #ifdef deb
                    printf("destination %s is valid, changing mode to 2\n", temp_entity->name);
                  #endif
                  mode = 2;
                  prev_entity = temp_entity;
                  temp_start = i;
                }
                else
                {
                  #ifdef deb
                    printf("destination %s is not valid, changing mode to 1\n", temp_entity->name);
                  #endif
                  mode = 1;
                  prev_entity = temp_entity;
                  temp_start = i;
                  i += 3;
                }
                break;
              }
              case 1: //burst delete
              {
                #ifdef deb
                  printf("mode 1:\n");
                #endif
                temp_entity = dest_array[i];
                if (temp_entity == prev_entity)
                {
                  #ifdef deb
                    printf("same destination as before (%s)\n", temp_entity->name);
                  #endif
                  i += 3;
                }
                else
                {
                  int step = i - temp_start;
                  #ifdef deb
                  for (int j=temp_start; j<i; j += 3)
                    printf("deleting %s --> %s\n", src_array[j]->name, dest_array[j]->name);
                  #endif
                  sub_rel_array_fixup_delete_complete_static(dest_array, temp_start, rel_num, step);
                  rel_num -= step;
                  i -= step;
                  if (check_entity_validity(temp_entity, del_array, del_num))
                  {
                    #ifdef deb
                      printf("destination %s is valid, changing mode to 2\n", temp_entity->name);
                    #endif
                    mode = 2;
                    prev_entity = temp_entity;
                    temp_start = i;
                  }
                  else
                  {
                    #ifdef deb
                      printf("destination %s is not valid, staying in mode 1\n", temp_entity->name);
                    #endif
                    prev_entity = temp_entity;
                    temp_start = i;
                    i += 3;
                  }
                }
                break;
              }
              case 2: //valid streak
              {
                #ifdef deb
                  printf("mode 2:\n");
                #endif
                temp_entity = dest_array[i];
                if (temp_entity == prev_entity)
                {
                  if (check_entity_validity(src_array[i], del_array, del_num))
                  {
                    #ifdef deb
                      printf("source %s is valid\n", src_array[i]->name);
                    #endif
                    ++ temp_counter;
                    i += 3;
                  }
                  else
                  {
                    #ifdef deb
                      printf("deleting %s --> %s\n", src_array[i]->name, dest_array[i]->name);
                    #endif
                    sub_rel_array_fixup_delete_complete(dest_array, temp_start, rel_num);
                    -- rel_num;
                  }
                }
                else
                {
                  #ifdef deb
                    printf("found a new destination\n");
                  #endif
                  if (check_entity_validity(temp_entity, del_array, del_num))
                  {
                    #ifdef deb
                      printf("destination %s is valid, staying in mode 2\n", temp_entity->name);
                    #endif
                    if (temp_counter == maximum)
                    {
                      rel_report_temp[rel_num_temp] = dest_name_array[i-3];
                      rel_num_temp ++;
                    }
                    else if (temp_counter > maximum)
                    {
                      rel_report_temp[0] = dest_name_array[i-3];
                      rel_num_temp = 1;
                      maximum = temp_counter;
                    }
                    temp_counter = 1;

                    prev_entity = temp_entity;
                    temp_start = i;
                  }
                  else
                  {
                    #ifdef deb
                      printf("destination %s is not valid, changing mode to 1\n", temp_entity->name);
                    #endif
                    mode = 1;
                    prev_entity = temp_entity;
                    temp_start = i;
                    i += 3;
                  }
                }
                break;
              }
            }
          }

          if (mode == 1)
          {
            rel_num -= i - temp_start;
          }

          if (rel_num > 0)
          {
            if (temp_counter == maximum)
            {
              rel_report_temp[rel_num_temp] = dest_name_array[i-3];
              ++ rel_num_temp;
            }
            else if (temp_counter > maximum)
            {
              rel_report_temp[0] = dest_name_array[i-3];
              rel_num_temp = 1;
              maximum = temp_counter;
            }

            relation->sub_relation_number = rel_num/3;
            p = fast_strcat(p, relation->name);
            p = fast_strcat(p, " ");
            for (int i=0; i<rel_num_temp; i++)
            {
              p = fast_strcat(p, *rel_report_temp[i]);
              p = fast_strcat(p, " ");
            }
            sprintf(tmp, "%d; ", maximum);
            p = fast_strcat(p, tmp);
            ++ h;
          }
          else
          {
            #ifdef deb
              printf("relation %s is unused, deleting (LATE)\n", relation->name);
            #endif
            delete_unused_relation(relations_buffer, relation, h, rr);
            -- rr;
            ++ sig_forget;
          }
        }
        else
        {
          #ifdef deb
            printf("relation %s is unused, deleting\n", relation->name);
          #endif
          delete_unused_relation(relations_buffer, relation, h, rr);
          -- rr;
          ++ sig_forget;
        }
        #ifdef deb
          printf("changing relation, new rel_num = %d\n\n", relation->sub_relation_number);
        #endif
      }
    }
    else
    {
      #ifdef deb
        printf("entities have NOT been deleted, cleaning up...\n");
      #endif
      int h = 0;
      while (h < rr)
      {
        relation = relations_buffer[h];
        rel_num = relation->sub_relation_number;
        if (rel_num > 0)
        {
          #ifdef deb
            printf("relation %s is valid, accessing sub-relations\n", relation->name);
          #endif
          rel_num_temp = 0;

          dest_array = (Entity**) relation->subrel_array;
          src_array = dest_array+1;
          dest_name_array = (String**) dest_array+2;

          prev_entity = dest_array[0];
          temp_counter = maximum = 0;
          for (k=0; k<rel_num; k+=3)
          {
            #ifdef deb
              printf("%d\n", k);
            #endif
            temp_entity = dest_array[k];
            if (prev_entity == temp_entity)
            {
              #ifdef deb
                printf("prev: %s, current: %s\n", prev_entity->name, temp_entity->name);
              #endif
              temp_counter ++;
            }
            else
            {
              if (temp_counter > maximum)
              {
                #ifdef deb
                  printf("found a bigger maximum\n");
                #endif
                maximum = temp_counter;
                rel_report_temp[0] = dest_name_array[k-3];
                rel_num_temp = 1;
              }
              else if (temp_counter == maximum)
              {
                #ifdef deb
                  printf("same maximum\n");
                #endif
                rel_report_temp[rel_num_temp] = dest_name_array[k-3];
                rel_num_temp ++;
              }

              #ifdef deb
                printf("%s, %d;\n", prev_entity->name, temp_counter);
              #endif
              prev_entity = temp_entity;
              temp_counter = 1;
              #ifdef deb
                printf("found a new entity, %s\n", temp_entity->name);
              #endif
            }
          }

          if (temp_counter > maximum)
          {
            maximum = temp_counter;
            rel_report_temp[0] = dest_name_array[k-3];
            rel_num_temp = 1;
          }
          else if (temp_counter == maximum)
          {
            rel_report_temp[rel_num_temp] = dest_name_array[k-3];
            rel_num_temp ++;
          }

          #ifdef deb
            printf("ARRAY %s(%d, %d):\n", relation->name, rel_num_temp, maximum);
            for (int i=0; i<rel_num_temp; i++)
              printf("%s\n", *rel_report_temp[i]);
            printf("\n");
          #endif

          p = fast_strcat(p, relation->name);
          p = fast_strcat(p, " ");
          for (int i=0; i<rel_num_temp; i++)
          {
            p = fast_strcat(p, *rel_report_temp[i]);
            p = fast_strcat(p, " ");
          }
          sprintf(tmp, "%d; ", maximum);
          p = fast_strcat(p, tmp);
          ++ h;
        }
        else
        {
          #ifdef deb
            printf("relation %s is unused, deleting\n", relation->name);
          #endif
          delete_unused_relation(relations_buffer, relation, h, rr);
          rr --;
          sig_forget ++;
        }
      }
      number_of_relations = rr;
    }
  }

  //acutally print the saved report string.
  puts(report_string);

  return sig_forget;
}
//does the report.

//debug functions---------------------------------------------------------------

#ifdef deb

void deb_print_entities(Entity** entity_table)
{
  Entity* entity;
  for (int j=0; j<ENTITY_TABLE_SIZE; j++)
  {
    if (entity_table[j] != NULL)
      printf("(%d) %s\n", j, entity_table[j]->name);
  }
}

void deb_print_sub_relations(Relation* rel)
{
  int i;
  int rel_num = rel->sub_relation_number;

  Entity** dest_array = (Entity**) rel->subrel_array;
  Entity** src_array = dest_array;
  String** dest_name_array = ((String**) dest_array);

  printf("%s:\n", rel->name);
  for(i=0; i<rel_num; i++)
    printf("%d: %s --> %s (%s)\n", i, src_array[i*3+1]->name, dest_array[i*3]->name, dest_name_array[i*3+2]);
}

void deb_print_relations(Relation* relations_buffer[RELATIONS_BUFFER_SIZE])
{
  for (int i=0; i<number_of_relations; i++)
  {
    if (relations_buffer[i]->sub_relation_number > 0)
      deb_print_sub_relations(relations_buffer[i]);
  }
}

#endif

//------------------------------------------------------------------------------
int main() //main program
{
  //main data structures
  static Entity* entity_table[ENTITY_TABLE_SIZE] = {NULL};
  Entity* deleted_entities_array[1024];
  int del_ent_num = 0;
  Relation* relations_buffer[RELATIONS_BUFFER_SIZE];
  //temp variables
  String argument0;
  String argument1;
  String argument2;
  LongString input_string;
  //optimization variables
  int prev_opcode = 5;
  String prev_arg = "";
  int report_needed = 1; // 0 = report has not changed, 1 = report has changed.
  int sig_forget = 0; //used to tell addrel/delrel if a relations has been deleted.
  int sig_forget_del = 0;
  #ifdef deb
    int command_counter = 0;
  #endif

  int opcode;
  while (1)
  {
    fgets_unlocked(input_string, sizeof(LongString), stdin);

    #ifdef deb
      printf("\nCOMMAND %d: %sPREVIOUS ARG: %s\nPREVIOUS OPCODE: %d\nCOLLISIONS: %d\n", command_counter, input_string, prev_arg, prev_opcode, collisions);
      command_counter ++;
    #endif
    switch (input_string[0])
    {
      case 'a': //addent or addrel
      {
        if (input_string[3] == 'e') //addent
        {
          get_argument(input_string, argument0, 8);
          if (prev_opcode != 0)
          {
            memcpy(prev_arg, argument0, sizeof(String));
            handle_entity_creation(entity_table, argument0);
            prev_opcode = 0;
          }
          else
          {
            if (strcmp(prev_arg, argument0) != 0)
            {
              memcpy(prev_arg, argument0, sizeof(String));
              handle_entity_creation(entity_table, argument0);
            }
          }
          #ifdef deb
            deb_print_entities(entity_table);
            deb_print_relations(relations_buffer);
          #endif
        }
        else //addrel
        {
          opcode = get_argument(input_string, argument0, 8);
          opcode = get_argument(input_string, argument1, opcode+2);
          get_argument(input_string, argument2, opcode+2);
          if (handle_relation_creation(entity_table, relations_buffer,  argument0, argument1, argument2, sig_forget) != 0)
          {
            report_needed = 1;
            prev_opcode = 1;
          }
          sig_forget = 0;
          #ifdef deb
            //deb_print_entities(entity_table);
            deb_print_relations(relations_buffer);
          #endif
        }
        break;
      }
      case 'd': //delent or delrel
      {
        if (input_string[3] == 'e') //delent
        {
          get_argument(input_string, argument0, 8);
          if (prev_opcode != 2)
          {
            Entity* ent = delent_function(entity_table, argument0);
            if (ent != 0)
            {
              deleted_entities_array[del_ent_num] = ent;
              del_ent_num ++;
              report_needed = 1;
              #ifdef deb
                printf("\ndeleted entity was in at least a relation!\n\n");
              #endif
            }
            prev_opcode = 2;
            memcpy(prev_arg, argument0, sizeof(String));
          }
          else
          {
            if (strcmp(prev_arg, argument0) != 0)
            {
              Entity* ent = delent_function(entity_table, argument0);
              if (ent != 0)
              {
                deleted_entities_array[del_ent_num] = ent;
                del_ent_num ++;
                report_needed = 1;
                #ifdef deb
                  printf("\ndeleted entity was in a relation!\n\n");
                #endif
              }
              memcpy(prev_arg, argument0, sizeof(String));
            }
          }
          #ifdef deb
            deb_print_entities(entity_table);
            deb_print_relations(relations_buffer);
          #endif
        }
        else //delrel
        {
          opcode = get_argument(input_string, argument0, 8);
          opcode = get_argument(input_string, argument1, opcode+2);
          get_argument(input_string, argument2, opcode+2);
          if (delrel_function(entity_table, relations_buffer, argument0, argument1, argument2, sig_forget_del) != 0)
          {
            report_needed = 1;
            prev_opcode = 3;
          }
          sig_forget_del = 0;
          #ifdef deb
            //deb_print_entities(entity_table);
            deb_print_relations(relations_buffer);
          #endif
        }
        break;
      }
      case 'r': //report
      {
        sig_forget += report_function(relations_buffer, report_needed, deleted_entities_array, del_ent_num);
        sig_forget_del += sig_forget;
        del_ent_num = 0;
        report_needed = 0;
        #ifdef deb
          printf("\nsig_forget = %d\n\n", sig_forget);
          deb_print_entities(entity_table);
          deb_print_relations(relations_buffer);
        #endif
        break;
      }
      case 'e':
      {
        #ifdef deb
          printf("execution ended correctly.\n");
        #endif
        return 0;
      }
    }
  }
}
