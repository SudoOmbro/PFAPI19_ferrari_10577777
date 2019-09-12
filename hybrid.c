#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTITY_TABLE_SIZE  711353 //711353
#define SUB_RELATIONS_ARRAY_SIZE 100000 //98304
#define RELATIONS_BUFFER_SIZE 1024
#define SRC_ARRAY_BUFFER 640

/* add -Ddeb to gcc compiler options to compile in verbose debug mode */

//type definitions--------------------------------------------------------------

typedef char SuperLongString[5632];
typedef char LongString[128];
typedef char String[40];

typedef struct {
  String name;
  int sub_relation_number;
  void* sub_rel_array[SUB_RELATIONS_ARRAY_SIZE];
} Relation;

typedef struct {
  unsigned long value;
  String name;
  int rel_num;
} Entity;

typedef struct {
  String dest_name;
  Entity* destination;
  int src_num;
  Entity* source_buffer[SRC_ARRAY_BUFFER];
} SubRelation;  //the relation is the relation that contains it.

int number_of_relations = 0;
#ifdef deb
  int collisions = 0;
#endif

//------------------------------------------------------------------------------
//functions definitions---------------------------------------------------------

inline __attribute__((always_inline)) unsigned long entity_hash_function(char* text)
{
  unsigned long hash = 0;
  int i = 1;

  while(text[i] != '"')
    hash = hash*131 + text[i++];

  #ifdef deb
    printf("entity hash: %lu\n", hash);
  #endif

  return hash;
}

char* get_argument(LongString input_string, char* dest_string)
{
  *dest_string = '"';
  while (*input_string != '"') //while the char is valid
    *++dest_string = *input_string++;
  *++dest_string = '"';
  *++dest_string = '\0';
  return input_string+3;
}
//given a string in stdin, it fills dest_string with the input_string
//until a ' " ' is found. Returns position in input string.

Entity* get_entity(Entity** entity_table, String name)
{
  #ifdef deb
    printf("getting entity %s.\n", name);
  #endif
  unsigned long value = entity_hash_function(name);
  int pos = value % ENTITY_TABLE_SIZE;
  if (entity_table[pos] != NULL)
    return entity_table[pos];
  return 0;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

int relations_binary_search(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name, int l, int r)
{
  int mid, val;
  while (r >= l)
  {
        #ifdef deb
          printf("binary search %d, %d\n", l, r);
        #endif
        mid = l + (r - l) / 2;
        val = strcmp(name, relations_buffer[mid]->name);
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

int sub_relations_binary_search(SubRelation** array, String name, int l, int r)
{
  #ifdef deb
    printf("looking for destination %s\n", name);
  #endif
  int mid;
  int out_val;
  while (r >= l)
  {
        #ifdef deb
          printf("sub rel binary search %d, %d\n", l, r);
        #endif
        mid = l + (r - l) / 2;
        out_val = strcmp(name, (array[mid]->dest_name));
        if (out_val == 0)
        {
          return mid;
        }
        else
        {
          #ifdef deb
            printf("nope.\n");
          #endif
          if (out_val < 0)
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
//looks for a sub relation, if it finds it it returns its postion, else
//it returns -1.

void sub_rel_array_fixup(void** array, int start_pos, const int max, void* replacer)
{
  int i;
  for (i=max; i>start_pos; i--)
    array[i] = array[i-1];
  array[i] = replacer;
}
//given the pointer to the array to restore, the starting position, the
//end postion and the entity that replaces, this function restores
//the order in the array.

void sub_rel_array_fixup_delete(void** array, int start_pos, int end)
{
  int i;
  for (i=start_pos; i<end; i++)
    array[i] = array[i+1];
  array[i] = NULL;
}
//given the pointer to the array to restore, the starting position
//and the end postion, this function restores the order in the array.

int sub_relations_binary_search_creation(SubRelation** array, String dest_name, int l, int r)
{
  int mid;
  int ext_val;
  while (r >= l)
  {
        #ifdef deb
          printf("binary search %d, %d\n", l, r);
        #endif
        mid = l + (r - l) / 2;
        ext_val = strcmp(dest_name, (array[mid]->dest_name));
        if (ext_val == 0)
        {
          #ifdef deb
            printf("Destination exists\n");
          #endif
          return mid;
        }
        else if (ext_val < 0)
        {
            #ifdef deb
              printf("int %s < %s\n", dest_name, (array[mid]->dest_name));
            #endif
            r = mid - 1;
        }
        else
        {
          #ifdef deb
            printf("int %s > %s\n", dest_name, (array[mid]->dest_name));
          #endif
          l = mid + 1;
        }
  }
  #ifdef deb
    printf("r(%d) < l(%d)\n", r, l);
  #endif
  return l;
}
//looks for a sub relation, if it finds it it returns its position,
//else it returns where to create it.

int src_binary_search(Entity** src_array, Entity* src, int l, int r)
{
  #ifdef deb
    printf("looking for source %s\n", src->name);
  #endif
  int mid;
  Entity* val;
  while (r >= l)
  {
    mid = l + (r - l) / 2;
    val = src_array[mid];
    if (val == src)
    {
      #ifdef deb
        printf("relation exists.\n");
      #endif
      return mid;
    }
    if (val < src)
      l = mid + 1;
    else
      r = mid - 1;
  }
  return -1;
}
//looks for an entity in the src_buffer of a relation, if it finds it it returns
//its position, else it returns -1.

int src_binary_search_creation(Entity** src_array, Entity* src, int l, int r)
{
  int size = r;
  int mid;
  Entity* val;
  while (r >= l)
  {
    mid = l + (r - l) / 2;
    val = src_array[mid];
    if (val == src)
    {
      #ifdef deb
        printf("relation already exists.\n");
      #endif
      return 0;
    }
    if (val < src)
      l = mid + 1;
    else
      r = mid - 1;
  }
  sub_rel_array_fixup((void**)src_array, l, size, src);
  ++ (src->rel_num);
  return 1;
}
//looks for an entity in the src_buffer of a relation, if it finds it it returns
//-1, else it returns the position where to create it.

Relation* get_relation(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name)
{
  #ifdef deb
    printf("getting relation %s.\n", name);
  #endif

  if (number_of_relations == 0)
    return 0;

  int val = relations_binary_search(relations_buffer, name, 0, number_of_relations-1);
  if (val != -1)
    return relations_buffer[val];
  else
    return 0;
}

Entity* create_entity(String name, unsigned long value)
{
  //create the entity in the hash table
  Entity* self;
  self = (Entity*) malloc(sizeof(Entity));
  strcpy(self->name, name);
  self->rel_num = 0;
  self->value = value;
  return self;
}
//create entity, initialize values.

void rel_buffer_fixup(Relation** array, int start_pos, Relation* replacer)
{
  int i;
  for (i=number_of_relations; i>start_pos; i--)
    array[i] = array[i-1];
  array[i] = replacer;
}
//fix the relations array when addition.

void rel_buffer_fixup_delete(Relation** relations_buffer, int start_pos)
{
  int i;
  for (i=start_pos; i<number_of_relations; i++)
    relations_buffer[i] = relations_buffer[i+1];
  relations_buffer[i] = NULL;
}
//fix the relatiobs array when deletion.

Relation* create_relation(String name)
{
  //create the relation to put in hash table
  Relation* self = (Relation*) malloc(sizeof(Relation));
  strcpy(self->name, name);
  self->sub_relation_number = 0;
  return self;
}
//Create relation in hash table, create relation in ordered list and
//return the pointer to the relation in hash table.

SubRelation* create_sub_relation(Entity* src, Entity* dest)
{
  #ifdef deb
    printf("creating sub-relation...\n");
  #endif
  SubRelation* self = (SubRelation*) malloc(sizeof(SubRelation));
  self->src_num = 1;
  *(self->source_buffer) = src;
  self->destination = dest;
  strcpy(self->dest_name, dest->name);
  ++ (src->rel_num);
  ++ (dest->rel_num);
  return self;
}
//creates a sub relation and returns the pointer to that.

int relation_add_entities(Relation* rel, Entity* src, Entity* dest)
{
  //setup
  int rel_num = rel->sub_relation_number;
  SubRelation* sub_rel;
  SubRelation** array = (SubRelation**) rel->sub_rel_array;
  if (rel_num == 0)
  {
    #ifdef deb
      printf("created sub relation in first slot of relation\n");
    #endif
    array[0] = create_sub_relation(src, dest);
    rel->sub_relation_number ++;
    return 1;
  }
  else
  {
    int mid_val = sub_relations_binary_search_creation(array, dest->name, 0, rel_num-1);
    #ifdef deb
      printf("finished binary search\n");
    #endif
    if (mid_val == rel_num)
    {
      sub_rel = create_sub_relation(src, dest);
      ++ rel_num;
      sub_rel_array_fixup( (void**) array, mid_val, rel_num, sub_rel);
      rel->sub_relation_number = rel_num;
      #ifdef deb
        printf("added sub new destination in last position (%d)\n", mid_val);
      #endif
      return 1;
    }
    if (array[mid_val]->destination == dest)
    {
      SubRelation* sub_rel = array[mid_val];
      if (src_binary_search_creation(sub_rel->source_buffer, src, 0, sub_rel->src_num) == 0)
      {
        return 0;
      }
      else
      {
        ++ (sub_rel->src_num);
        ++ (dest->rel_num);
        return 1;
      }
    }
    else
    {
      #ifdef deb
        printf("found a spot\n");
      #endif
      sub_rel = create_sub_relation(src, dest);
      ++ rel_num;
      sub_rel_array_fixup( (void**) array, mid_val, rel_num, sub_rel);
      rel->sub_relation_number = rel_num;
      #ifdef deb
        printf("added sub new destination in %d\n", mid_val);
      #endif
      return 1;
    }
    return 0;
  }
}
//adds entities to the new/existing relation;
//return 0 if everything went okay, else 1.

inline __attribute__((always_inline)) int handle_entity_creation(Entity** entity_table, String name)
{
  //do the hash functio but also save the value of the entity
  unsigned long value = entity_hash_function(name);
  int pos = (value) % ENTITY_TABLE_SIZE;
  #ifdef deb
    printf("entity value: %d\n", value);
    printf("entity hash value: %d\n", (value) % ENTITY_TABLE_SIZE);
  #endif

  if (entity_table[pos] == NULL)
  {
    #ifdef deb
      printf("Created new entity\n");
    #endif
    entity_table[pos] = create_entity(name, value);
    return 1;
  }
  #ifdef deb
    printf("Entity already exists\n");
    if (value != entity_table[pos]->value)
      collisions ++;
  #endif
  return 0;
}
//handle entity creation, return the created entity if success, else 0

int handle_relation_creation(Entity** entity_table, Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name1, String name2, String name)
{
  Entity* e1 = get_entity(entity_table, name1);
  if (e1 == 0) //check if entities exist.
  {
    #ifdef deb
      printf("one of the entities does not exist\n");
    #endif
    return 0;
  }
  Entity* e2 = get_entity(entity_table, name2);
  if (e2 == 0) //check if entities exist.
  {
    #ifdef deb
      printf("one of the entities does not exist\n");
    #endif
    return 0;
  }

  #ifdef deb
    printf("all of the entities exist\n");
  #endif

  if (number_of_relations == 0)
  {
    relations_buffer[0] = create_relation(name);
    number_of_relations ++;
    return relation_add_entities(relations_buffer[0], e1, e2);
  }

  int r = number_of_relations-1, l = 0, val, mid;

  while (r >= l)
  {
        #ifdef deb
          printf("binary search %d, %d\n", l, r);
        #endif
        mid = l + (r - l) / 2;
        val = strcmp(name, relations_buffer[mid]->name);
        if (val == 0)
        {
            #ifdef deb
              printf("found relation %s.\n", name);
            #endif
            return relation_add_entities(relations_buffer[mid], e1, e2);
        }
        if (val < 0)
            r = mid - 1;
        else
            l = mid + 1;
  }

  #ifdef deb
    printf("created new relation in %d", l);
  #endif

  Relation* self = create_relation(name);
  number_of_relations ++;
  rel_buffer_fixup(relations_buffer, l, self);
  return relation_add_entities(self, e1, e2);
}
//handle relation creation, return 0 if nothing was created, the relation
//created or modified otherwise.

inline __attribute__((always_inline)) Entity* delent_function(Entity** entity_table, String name)
{
  unsigned long value = entity_hash_function(name);
  int pos = value % ENTITY_TABLE_SIZE;
  Entity* entity = entity_table[pos];
  if (entity_table[pos] == NULL)
  {
    #ifdef deb
      printf("invalid entity.\n");
    #endif
    return 0;
  }
  entity_table[pos] = NULL;
  #ifdef deb
    printf("deallocated entity at %x\n", entity);
  #endif
  if (entity->rel_num != 0)
    return entity;
  return 0;
}
//deallocates an entity; return 1 if succes, else 0

void delete_unused_relation(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], Relation* rel, int start_pos)
{
  //relations_table[hash][pos] = NULL;
  number_of_relations --;
  rel_buffer_fixup_delete(relations_buffer, start_pos);
  free(rel);
}

inline __attribute__((always_inline)) int delrel_function(Entity** entity_table, Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name_source, String name_dest, String rel_name)
{
  //check if relation exists
  Relation* rel = get_relation(relations_buffer, rel_name);
  if (rel == 0)
  {
    #ifdef deb
      printf("relation does not exist.\n");
    #endif
    return 0;
  }
  //check if source exists
  Entity* src = get_entity(entity_table, name_source);
  if(src == 0) //check if entities and relation exist.
  {
    #ifdef deb
      printf("one of the entities does not exist.\n");
    #endif
    return 0;
  }
  //check if destination exists
  Entity* dest = get_entity(entity_table, name_dest);
  if(dest == 0) //check if entities and relation exist.
  {
    #ifdef deb
      printf("one of the entities does not exist.\n");
    #endif
    return 0;
  }

  #ifdef deb
    printf("relation may exist.\n");
  #endif

  SubRelation* sub_rel;
  int i, j;
  SubRelation** array;

  array = (SubRelation**) rel->sub_rel_array;
  i = sub_relations_binary_search(array, name_dest, 0, rel->sub_relation_number-1);
  if (i != -1)
  {
      sub_rel = array[i];
      Entity** src_array = array[i]->source_buffer;
      j = src_binary_search(src_array, src, 0, array[i]->src_num-1);
      if (j != -1)
      {
        if (array[i]->src_num == 1)
        {
          free(sub_rel);
          sub_rel_array_fixup_delete((void**)array, i, --rel->sub_relation_number);
          #ifdef deb
            printf("deleted sub relation %s %s\n", name_source, name_dest);
          #endif
        }
        else
        {
          sub_rel_array_fixup_delete((void**)src_array, j, --array[i]->src_num);
          #ifdef deb
            printf("deleted source %s in sub relation %s\n", name_source, name_dest);
          #endif
        }
        src->rel_num --;
        dest->rel_num --;
        return 1;
      }
      #ifdef deb
        printf("source does not exist (LATE)\n");
      #endif
      return 0;
  }
  #ifdef deb
    printf("destination does not exist (LATE)\n");
  #endif
  return 0;
}
//deletes a sub relation; return 1 if success, else 0

int check_entity_validity(Entity* entity, Entity** array, int del_num)
{
  #ifdef deb
    printf("checkin validity of %s\n", entity->name);
  #endif
  while (del_num != 0)
  {
    #ifdef deb
      printf("%d ? %s", del_num-1, array[del_num-1]->name);
    #endif
    if (entity == array[--del_num])
    {
      #ifdef deb
        printf(" found match, entity is not valid.\n");
      #endif
      return 0;
    }
    #ifdef deb
      printf(" nope\n");
    #endif
  }
  return 1;
}
//checks if the given entity hasn't been deleted.

char* fast_strcat(char* dest, char* src)
{
  while (*dest++ = *src++);
  return --dest;
}

inline __attribute__((always_inline)) int report_function(Relation** relations_buffer, int update, Entity** del_array, int del_num)
{
  //if there are no relations, do nothing
  if (number_of_relations == 0)
  {
    puts("none");
    return 0;
  }

  static SuperLongString report_string;

  if (update == 1) //the report needs to be updated
  {
    char* rp = report_string;
    Relation* relation;
    SubRelation* sub_rel;
    SubRelation** sub_rel_array;
    char tmp[20];
    int rel_num;
    int temp_counter = 0;
    int maximum;
    SubRelation* report_array[64];

    if (del_num > 0) //entities have been deleted.
    {
      int rr = number_of_relations;
      for (int i=0; i<rr; i++)
      {
        relation = relations_buffer[i];
        rel_num = relation->sub_relation_number;
        sub_rel_array = (SubRelation**) relation->sub_rel_array;
        maximum = 0;
        temp_counter = 0;
        #ifdef deb
          printf("checking relation %s ---------------\n", relation->name);
        #endif
        if (rel_num > 0)
        {
          Entity** src_array;
          int src_num;

          #ifdef deb
            for(int i=0; i<(relation->sub_relation_number); i++)
              printf("(%d) %s\n", i, sub_rel_array[i]->dest_name);
          #endif

          for (int j=0; j<rel_num; j++)
          {
            sub_rel = sub_rel_array[j];
            #ifdef deb
              printf("[%d]: checking %s for subrelations\n", j, (sub_rel->destination)->name);
            #endif
            if (check_entity_validity(sub_rel->destination, del_array, del_num) == 0)
            {
              free(sub_rel);
              sub_rel_array_fixup_delete((void**)sub_rel_array, j, --(relation->sub_relation_number));
              -- rel_num;
              #ifdef deb
                printf("deleted sub relation, destination was not valid. (%d, %d)\n", j-1, (relation->sub_relation_number));
                for(int i=0; i<(relation->sub_relation_number); i++)
                  printf("(%d) %s\n", i, sub_rel_array[i]->dest_name);
              #endif
              src_num = 0;
              -- j;
            }
            else
            {
              #ifdef deb
                printf("%s is valid, checking sources...\n", (sub_rel->destination)->name);
              #endif
              src_num = sub_rel->src_num;
              if (src_num != 0)
              {
                src_array = sub_rel->source_buffer;
                for (int k=0; k<src_num; k++)
                {
                  if (check_entity_validity(src_array[k], del_array, del_num) == 0)
                  {
                    #ifdef deb
                      printf("source %s is NOT valid\n", src_array[k]->name);
                    #endif
                    sub_rel_array_fixup_delete((void**)src_array, k, --src_num);
                    -- k;
                  }
                  else
                  {
                    #ifdef deb
                      printf("source %s is valid\n", src_array[k]->name);
                    #endif
                  }
                }
                sub_rel->src_num = src_num;
              }
              else
              {
                free(sub_rel);
                sub_rel_array_fixup_delete((void**)sub_rel_array, j, --(relation->sub_relation_number));
                -- rel_num;
                #ifdef deb
                  printf("deleted sub relation, destination was not valid. (%d, %d)\n", j-1, (relation->sub_relation_number));
                  for(int i=0; i<(relation->sub_relation_number); i++)
                    printf("(%d) %s\n", i, sub_rel_array[i]->dest_name);
                #endif
                src_num = 0;
                -- j;
              }
            }
            if (src_num != 0)
            {
              if (src_num > maximum)
              {
                maximum = src_num;
                report_array[0] = sub_rel;
                temp_counter = 1;
              }
              else if (src_num == maximum)
                report_array[temp_counter++] = sub_rel;
            }
          }
          #ifdef deb
            printf("finished creating data structure, (max: %d, num: %d) adding to string...\n", maximum, temp_counter);
            printf("ARRAY %s(%d, %d):\n", relation->name, temp_counter, maximum);
            for (int i=0; i<temp_counter; i++)
              printf("%s\n", report_array[i]->dest_name);
            printf("\n");
          #endif
          if (temp_counter != 0)
          {
            rp = fast_strcat(rp, relation->name);
            for (int i=0; i<temp_counter; i++)
            {
              *rp++ = ' ';
              rp = fast_strcat(rp, report_array[i]->dest_name);
            }
            *rp++ = ' ';
            sprintf(tmp, "%d; ", maximum);
            rp = fast_strcat(rp, tmp);
          }
          #ifdef deb
            printf("%s\n", report_string);
          #endif
        }
        else
        {
          #ifdef deb
            printf("relation %s is unused, deleting\n", relation->name);
          #endif
          delete_unused_relation(relations_buffer, relation, i);
          i --;
          rr --;
        }
        #ifdef deb
          printf("\n");
        #endif
      }
      *rp++ = '\n';
      *rp = '\0';
    }
    else //entites haven't been deleted.
    {
      int rr = number_of_relations;
      for (int i=0; i<rr; i++)
      {
        relation = relations_buffer[i];
        rel_num = relation->sub_relation_number;
        sub_rel_array = (SubRelation**) relation->sub_rel_array;
        maximum = 0;
        temp_counter = 0;
        #ifdef deb
          printf("checking relation %s ---------------\n", relation->name);
        #endif
        if (rel_num > 0)
        {
          Entity** src_array;
          int src_num;

          #ifdef deb
            for(int i=0; i<(relation->sub_relation_number); i++)
              printf("(%d) %s\n", i, sub_rel_array[i]->dest_name);
          #endif

          for (int j=0; j<rel_num; j++)
          {
              sub_rel = sub_rel_array[j];
              src_num = sub_rel->src_num;
              if (src_num == 0)
              {
                free(sub_rel);
                sub_rel_array_fixup_delete((void**)sub_rel_array, j, --(relation->sub_relation_number));
                -- rel_num;
                #ifdef deb
                  printf("deleted sub relation, destination was not valid. (%d, %d)\n", j-1, (relation->sub_relation_number));
                  for(int i=0; i<(relation->sub_relation_number); i++)
                    printf("(%d) %s\n", i, sub_rel_array[i]->dest_name);
                #endif
                src_num = 0;
                -- j;
              }
              else
              {
                if (src_num > maximum)
                {
                  maximum = src_num;
                  report_array[0] = sub_rel;
                  temp_counter = 1;
                }
                else if (src_num == maximum)
                  report_array[temp_counter++] = sub_rel;
              }
          }
          #ifdef deb
            printf("finished creating data structure, (max: %d, num: %d) adding to string...\n", maximum, temp_counter);
            printf("ARRAY %s(%d, %d):\n", relation->name, temp_counter, maximum);
            for (int i=0; i<temp_counter; i++)
              printf("%s\n", report_array[i]->dest_name);
            printf("\n");
          #endif
          if (temp_counter != 0)
          {
            rp = fast_strcat(rp, relation->name);
            for (int i=0; i<temp_counter; i++)
            {
              *rp++ = ' ';
              rp = fast_strcat(rp, report_array[i]->dest_name);
            }
            *rp++ = ' ';
            sprintf(tmp, "%d; ", maximum);
            rp = fast_strcat(rp, tmp);
          }
          #ifdef deb
            printf("%s\n", report_string);
          #endif
        }
        else
        {
          #ifdef deb
            printf("relation %s is unused, deleting\n", relation->name);
          #endif
          delete_unused_relation(relations_buffer, relation, i);
          i --;
          rr --;
        }
        #ifdef deb
          printf("\n");
        #endif
      }
      *rp++ = '\n';
      *rp = '\0';
    }
  }
  #ifdef deb
    printf("actual report string:\n");
  #endif
  if (*report_string != '\n')
    fputs_unlocked(report_string, stdout);
  else
    puts("none");
  return 1;
}

//debug functions---------------------------------------------------------------

#ifdef deb

void deb_print_entities(Entity** entity_table)
{
  for (int i=0; i<ENTITY_TABLE_SIZE; i++)
  {
    if (entity_table[i] != NULL)
      printf("(%d) %s, %lu\n", i, entity_table[i]->name, entity_table[i]->value);
  }
}

void deb_print_sub_relations(Relation* rel)
{
  int rel_num = rel->sub_relation_number;
  SubRelation** array = (SubRelation**) rel->sub_rel_array;
  for(int i=0; i<rel_num; i++)
  {
      if (array[i] != NULL)
      {
        Entity** src_buffer = array[i]->source_buffer;
        Entity* dest = array[i]->destination;
        int src_num = array[i]->src_num;
        for (int j=0; j<src_num; j++)
        {
          printf("%d (%d): %s (%d) --%s--> %s (%d)\n", i, j, src_buffer[j]->name,
          src_buffer[j]->rel_num, rel->name, dest->name, dest->rel_num);
        }
      }
      else
        printf("%d: NULLED\n", i);
  }
}

void deb_print_relations(Entity** entity_table, Relation** relations_buffer)
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
  static Entity* entity_table[ENTITY_TABLE_SIZE];
  Entity* deleted_entities_array[1024];
  int del_ent_num = 0;
  Relation* relations_buffer[RELATIONS_BUFFER_SIZE];
  //aux variables
  String argument0;
  String argument1;
  String argument2;
  LongString input_string;
  int report_needed = 1; // 0 = report has not changed, 1 = report has changed.
  #ifdef deb
    int command_counter = 0;
  #endif

  while (1)
  {
    fgets_unlocked(input_string, sizeof(LongString), stdin);
    char* p;

    #ifdef deb
      printf("\nCOMMAND %d: %sCOLLISIONS: %d\n\n", command_counter, input_string, collisions);
      command_counter ++;
    #endif
    switch (input_string[0]) {
    case 'a': //addent or addrel
    {
      if (input_string[3] == 'e') //addent
      {
        get_argument(input_string+8, argument0);
        handle_entity_creation(entity_table, argument0);
        #ifdef deb
          deb_print_entities(entity_table);
          deb_print_relations(entity_table, relations_buffer);
        #endif
      }
      else //addrel
      {
        p = get_argument(input_string+8, argument0);
        p = get_argument(p, argument1);
        get_argument(p, argument2);
        if (handle_relation_creation(entity_table, relations_buffer,  argument0, argument1, argument2) != 0)
          report_needed = 1;
        #ifdef deb
          //deb_print_entities(entity_table);
          deb_print_relations(entity_table, relations_buffer);
        #endif
      }
      break;
    }
    case 'd': //delent or delrel
    {
      if (input_string[3] == 'e') //delent
      {
        get_argument(input_string+8, argument0);
        Entity* ent = delent_function(entity_table, argument0);
        if (ent != 0)
        {
          deleted_entities_array[del_ent_num] = ent;
          del_ent_num ++;
          report_needed = 1;
        }
        #ifdef deb
          deb_print_entities(entity_table);
          deb_print_relations(entity_table, relations_buffer);
          if (del_ent_num > 0)
          {
            printf("deleted entitites array:\n");
            for (int i=0; i<del_ent_num; i++)
              printf("%s\n", deleted_entities_array[i]->name);
          }
        #endif
      }
      else //delrel
      {
        p = get_argument(input_string+8, argument0);
        p = get_argument(p, argument1);
        get_argument(p, argument2);
        if (delrel_function(entity_table, relations_buffer, argument0, argument1, argument2) != 0)
          report_needed = 1;
        #ifdef deb
          deb_print_entities(entity_table);
          deb_print_relations(entity_table, relations_buffer);
        #endif
      }
      break;
    }
    case 'r': //report
    {
      if (report_function(relations_buffer, report_needed, deleted_entities_array, del_ent_num) != 0)
      {
        del_ent_num = 0;
        report_needed = 0;
      }
      #ifdef deb
        printf("\n");
        deb_print_entities(entity_table);
        deb_print_relations(entity_table, relations_buffer);
      #endif
      break;
    }
    case 'e':
    {
      return 0;
    }

    }
  }
}
