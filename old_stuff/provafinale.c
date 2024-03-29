#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTITY_TABLE_SIZE  6451 //6451
#define SUB_RELATIONS_ARRAY_SIZE 131072 //78024
#define RELATIONS_BUFFER_SIZE 4096
#define COLLISION_BUFFER_SIZE 16 //32

/* add -Ddeb to gcc compiler options to compile in verbose debug mode */

//type definitions--------------------------------------------------------------

typedef char SuperLongString[5632];
typedef char LongString[128];
typedef char String[32];

typedef struct {
  String name;
  int sub_relation_number;
  void* sub_rel_array[SUB_RELATIONS_ARRAY_SIZE];
} Relation;

typedef struct {
  unsigned long source_value;
  unsigned long destination_value;
  String dest_name;
} SubRelation;  //the relation is the relation that contains it.

//------------------------------------------------------------------------------
//functions definitions---------------------------------------------------------

unsigned long entity_hash_function(char* text)
{
  unsigned long value = 0;
    for (int i=1; text[i] != '"'; i++)
      value = 131*value + text[i];
  return value;
}
//genereates the unique value for the given word

int get_argument(LongString input_string, char* dest_string, int start_pos)
{
  char byte = input_string[start_pos];
  dest_string[0] = '"';
  int string_length = 1;
  while (byte != '"') //while the char is valid
  {
    dest_string[string_length] = byte;
    string_length ++;
    start_pos ++;
    byte = input_string[start_pos];
  }
  dest_string[string_length] = '"';
  dest_string[string_length+1] = '\0';
  return start_pos;
}
//given a string in stdin, it fills dest_string with the input_string
//until a ' " ' is found. Returns position in input string.

void get_argument_no_return(LongString input_string, char* dest_string, int start_pos)
{
  char byte = input_string[start_pos];
  dest_string[0] = '"';
  int string_length = 1;
  while (byte != '"') //while the char is valid
  {
    dest_string[string_length] = byte;
    string_length ++;
    start_pos ++;
    byte = input_string[start_pos];
  }
  dest_string[string_length] = '"';
  dest_string[string_length+1] = '\0';
}
//given a string in stdin, it fills dest_string with the input_string
//until a ' " ' is found. Does not return anything.

int entity_hash_table_linear_search(unsigned long* array, unsigned long value)
{
  int i;
  #ifdef deb
    printf("starting linear search\n");
  #endif
  for(i=0; array[i] != 0; i++)
  {
    if (value == array[i])
    {
      #ifdef deb
        printf("linear search done, found entity\n");
      #endif
      return value;
    }
  }
  #ifdef deb
    printf("linear search done, no result\n");
  #endif
  return 0;
}

int entity_hash_table_linear_search_pos(unsigned long* array , unsigned long value)
{
  int i;
  #ifdef deb
    printf("starting linear search\n");
  #endif
  for(i=0; array[i] != 0; i++)
  {
    if (value == array[i])
    {
      #ifdef deb
        printf("linear search done, found entity\n");
      #endif
      return i;
    }
  }
  #ifdef deb
    printf("linear search done, no result\n");
  #endif
  return -1;
}

unsigned long get_entity(unsigned long entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE], String name)
{
  #ifdef deb
    printf("getting entity %s.\n", name);
  #endif
  unsigned long value = entity_hash_function(name);
  int pos = value % ENTITY_TABLE_SIZE;
  if (entity_table[pos][0] != 0)
    return entity_hash_table_linear_search(entity_table[pos], value);
  return 0;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

int relations_binary_search(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name, int l, int r)
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

int sub_relations_binary_search(SubRelation** array, String name, int l, int r, unsigned long src_val)
{
  while (r >= l)
  {
        #ifdef deb
          printf("sub rel binary search %d, %d\n", l, r);
        #endif
        int mid = l + (r - l) / 2;
        int out_val = strcmp(name, (array[mid]->dest_name));
        if (out_val == 0)
        {
            #ifdef deb
              printf("checking entity %s.\n", (array[mid]->dest_name));
            #endif
            unsigned long val = array[mid]->source_value;
            if (val == src_val)
            {
              return mid;
            }
            if (val > src_val)
                r = mid - 1;
            else
                l = mid + 1;
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

int sub_relations_binary_search_creation(SubRelation** array, String dest_name, int l, int r, unsigned long src_value)
{
  while (r >= l)
  {
        #ifdef deb
          printf("binary search %d, %d\n", l, r);
        #endif
        int mid = l + (r - l) / 2;
        int ext_val = strcmp(dest_name, (array[mid]->dest_name));
        if (ext_val == 0)
        {
            unsigned long val = array[mid]->source_value;
            if (val == src_value)
            {
              #ifdef deb
                printf("relation already exists.\n");
              #endif
              return -1;
            }
            if (val < src_value)
              l = mid + 1;
            else
              r = mid - 1;
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

Relation* get_relation(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name, int number_of_relations)
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

void sub_rel_array_fixup(SubRelation** array, int start_pos, const int max, SubRelation* replacer)
{
  int i;
  for (i=max; i>start_pos; i--)
    array[i] = array[i-1];
  array[i] = replacer;
}
//given the pointer to the array to restore, the starting position, the
//end postion and the entity that replaces, this function restores
//the order in the array.

void rel_buffer_fixup(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], int start_pos, Relation* replacer, int number_of_relations)
{
  Relation* temp;
  for (int i=start_pos; i<number_of_relations; i++)
  {
    temp = relations_buffer[i];
    relations_buffer[i] = replacer;
    replacer = temp;
  }
}

void sub_rel_array_fixup_delete(SubRelation** array, int start_pos, int end)
{
  int i;
  for (i=start_pos; i<end; i++)
    array[i] = array[i+1];
  array[i] = NULL;
}
//given the pointer to the array to restore, the starting position
//and the end postion, this function restores the order in the array.

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

SubRelation* create_sub_relation(unsigned long src, unsigned long dest, String dest_name)
{
  #ifdef deb
    printf("creating sub-relation...\n");
  #endif
  SubRelation* self = (SubRelation*) malloc(sizeof(SubRelation));
  self->source_value = src;
  self->destination_value = dest;
  #ifdef deb
    printf("doing memcpy...\n");
  #endif
  memcpy(self->dest_name, dest_name, sizeof(String));
  #ifdef deb
    printf("memcpy done\n");
  #endif
  return self;
}
//creates a sub relation and returns the pointer to that.

int relation_add_entities(Relation* rel, unsigned long src, unsigned long dest, String dest_name)
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
    array[0] = create_sub_relation(src, dest, dest_name);
    rel->sub_relation_number ++;
    return 1;
  }
  else
  {
    int mid_val = sub_relations_binary_search_creation(array, dest_name, 0, rel_num-1, src);
    if (mid_val > -1)
    {
      #ifdef deb
        printf("found a spot\n");
      #endif
      sub_rel = create_sub_relation(src, dest, dest_name);
      rel_num ++;
      sub_rel_array_fixup(array, mid_val, rel_num, sub_rel);
      rel->sub_relation_number = rel_num;
      #ifdef deb
        printf("added sub relation in %d\n", mid_val);
      #endif
      return 1;
    }
    return 0;
  }
}
//adds entities to the new/existing relation;
//return 0 if everything went okay, else 1.

int handle_entity_creation(unsigned long entity_table[][COLLISION_BUFFER_SIZE], unsigned long value)
{
  #ifdef deb
    printf("entity value: %lu\n", value);
    printf("entity hash value: %lu\n", (value) % ENTITY_TABLE_SIZE);
  #endif

  int pos = (value) % ENTITY_TABLE_SIZE;

    int i;
    unsigned long* array = entity_table[pos];
    for(i=0; array[i] != 0; i++)
    {
      if (value == array[i])
      {
        #ifdef deb
          printf("Entity already exists\n");
        #endif
        return 0;
      }
    }

    #ifdef deb
      printf("Created new entity\n");
    #endif
    entity_table[pos][i] = value;
    return 1;
}
//handle entity creation, return the created entity if success, else 0

int handle_relation_creation(unsigned long entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE], Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name1, String name2, String name, int number_of_relations)
{
  unsigned long e1 = get_entity(entity_table, name1);
  unsigned long e2 = get_entity(entity_table, name2);

  if (e1 == 0 || e2 == 0) //check if entities exist.
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
    relation_add_entities(relations_buffer[0], e1, e2, name2);
    return 2;
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
            return relation_add_entities(relations_buffer[mid], e1, e2, name2);
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
  rel_buffer_fixup(relations_buffer, l, self, number_of_relations+1);
  relation_add_entities(self, e1, e2, name2);
  return 2;
}
//handle relation creation, return 0 if nothing was created, the relation
//created or modified otherwise.

void entity_table_fixup_delete(unsigned long* array, int start)
{
  int i;
  for (i=start; array[i+1] != 0; i++)
    array[i] = array[i+1];
  array[i] = 0;
}

unsigned long delent_function(unsigned long entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE], unsigned long entity)
{
  int pos = entity % ENTITY_TABLE_SIZE;
  int off = entity_hash_table_linear_search_pos(entity_table[pos], entity);
  if (off == -1)
  {
    #ifdef deb
      printf("invalid entity.\n");
    #endif
    return 0;
  }
  entity_table_fixup_delete(entity_table[pos], off);
  #ifdef deb
    printf("deallocated entity with id: %lu\n", entity);
  #endif
  return entity;
}
//deallocates an entity; return 1 if succes, else 0

int delrel_function(unsigned long entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE], Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name_source, String name_dest, String rel_name, int number_of_relations)
{
  Relation* rel = get_relation(relations_buffer, rel_name, number_of_relations);

  if (rel == 0)
  {
    #ifdef deb
      printf("relation does not exist.\n");
    #endif
    return 0;
  }

  unsigned long src = get_entity(entity_table, name_source);
  unsigned long dest = get_entity(entity_table, name_dest);

  if(src == 0 || dest == 0) //check if entities and relation exist.
  {
    #ifdef deb
      printf("one of the entities does not exist.\n");
    #endif
    return 0;
  }

  SubRelation* sub_rel;
  int i;
  SubRelation** array;

  //delete sub relation from relation list in relation
  #ifdef deb
    printf("relation may exist.\n");
  #endif
  array = (SubRelation**) rel->sub_rel_array;
  i = sub_relations_binary_search(array, name_dest, 0, rel->sub_relation_number-1, src);
  if (i > -1)
  {
      sub_rel = array[i];
      free(sub_rel);
      rel->sub_relation_number --;
      sub_rel_array_fixup_delete(array, i, rel->sub_relation_number);
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

int check_sub_rel_validity(SubRelation* sub_rel, unsigned long del_array[1024], int del_num)
{
  unsigned long dest = sub_rel->destination_value;
  unsigned long src = sub_rel->source_value;
  int check = 0;

  for (int i=0; i<del_num; i++)
  {
    check += (dest == del_array[i]);
    check += (src == del_array[i]);
    if (check != 0)  return 0;
  }
  return 1;
}

int report_function(unsigned long entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE], Relation* relations_buffer[RELATIONS_BUFFER_SIZE], int update, unsigned long* del_array, int del_num, int number_of_relations)
{
  //if there are no relations, do nothing
  if (number_of_relations == 0)
  {
    printf("none\n");
    return 0;
  }

  static SuperLongString report_string;

  if (update == 1) //if the report needs to be updated:
  {
    //report string
    SuperLongString entities_string;
    strcpy(report_string, "");

    //aux variables
    unsigned long prev_entity;
    unsigned long temp_entity;
    Relation* relation;
    SubRelation* sub_rel;
    SubRelation* prev_rel;
    SubRelation** array;
    char tmp[20];
    int i = 0, k, temp_counter = 0;
    int maximum;
    int rel_num;

  if (del_num > 0) {

  #ifdef deb
    printf("entities have been deleted.\n");
  #endif

  for (int h=0; h<number_of_relations; h++)
  {
      relation = relations_buffer[h];
      rel_num = relation->sub_relation_number;
      if (rel_num > 0)
      {
        #ifdef deb
          printf("relation %s is valid, accessing sub-relations\n", relation->name);
        #endif
        array = (SubRelation**) relation->sub_rel_array;
        prev_rel = array[0];
        prev_entity = prev_rel->destination_value;
        temp_counter = maximum = 0;

        for (k=0; k<rel_num; k++)
        {
          sub_rel = array[k];
          if (check_sub_rel_validity(sub_rel, del_array, del_num) == 1)
          {
            temp_entity = sub_rel->destination_value;
            #ifdef deb
              printf("prev: %lu, current: %lu\n", prev_entity, temp_entity);
            #endif
            if (temp_entity == prev_entity)
            {
              #ifdef deb
                printf("increasing temp_counter\n");
              #endif
              temp_counter ++;
            }
            else
            {
                if (temp_counter > maximum)
                {
                  maximum = temp_counter;
                  memcpy(entities_string, prev_rel->dest_name, sizeof(String));
                  strcat(entities_string, " ");
                }
                else if (temp_counter == maximum)
                {
                  strcat(entities_string, prev_rel->dest_name);
                  strcat(entities_string, " ");
                }
                #ifdef deb
                  printf("%d: %s, %d;\n", i, prev_rel->dest_name, temp_counter);
                #endif
                prev_entity = temp_entity;
                prev_rel = sub_rel;
                temp_counter = 1;
                #ifdef deb
                  printf("found a new entity, %lu\n", temp_entity);
                #endif
            }
          }
          else
          {
            free(array[k]);
            relation->sub_relation_number --;
            rel_num --;
            sub_rel_array_fixup_delete(array, k, rel_num);
            k--;
            #ifdef deb
              printf("deleted relation at position %d\n", k);
            #endif
          }
        }

        if (temp_counter > maximum)
        {
          maximum = temp_counter;
          memcpy(entities_string, prev_rel->dest_name, sizeof(String));
          strcat(entities_string, " ");
        }
        else if (temp_counter != 0 && temp_counter == maximum)
        {
          strcat(entities_string, prev_rel->dest_name);
          strcat(entities_string, " ");
        }

        if (maximum > 0)
        {
          strcat(report_string, relation->name);
          strcat(report_string, " ");
          strcat(report_string, entities_string);
          sprintf(tmp, "%d; ", maximum);
          strcat(report_string, tmp);
          i ++;
          #ifdef deb
            printf("maximum is > 0, added relation to report array.\n");
          #endif
        }
    }
    else
    {
    #ifdef deb
      printf("relation %s is unused, deleting\n", relation->name);
    #endif
      h --;
    }
  }

  } else {

  #ifdef deb
    printf("entities have NOT been deleted.\n");
  #endif

  for (int h=0; h<number_of_relations; h++)
  {
      relation = relations_buffer[h];
      rel_num = relation->sub_relation_number;
      if (rel_num > 0)
      {
        #ifdef deb
          printf("relation %s is valid, accessing sub-relations\n", relation->name);
        #endif
        temp_counter = maximum = 0;
        array = (SubRelation**) relation->sub_rel_array;
        prev_rel = array[0];
        prev_entity = prev_rel->destination_value;
        for (k=0; k<rel_num; k++)
        {
            sub_rel = array[k];
            temp_entity = sub_rel->destination_value;
            #ifdef deb
              printf("prev: %lu, current: %lu\n", prev_entity, temp_entity);
            #endif
            if (temp_entity == prev_entity)
            {
              #ifdef deb
                printf("increasing temp_counter\n");
              #endif
              temp_counter ++;
            }
            else
            {
                if (temp_counter > maximum)
                {
                  maximum = temp_counter;
                  memcpy(entities_string, prev_rel->dest_name, sizeof(String));
                  strcat(entities_string, " ");
                }
                else if (temp_counter == maximum)
                {
                  strcat(entities_string, prev_rel->dest_name);
                  strcat(entities_string, " ");
                }
                #ifdef deb
                  printf("%d: %s, %d;\n", i, prev_rel->dest_name, temp_counter);
                #endif
                prev_entity = temp_entity;
                prev_rel = sub_rel;
                temp_counter = 1;
                #ifdef deb
                  printf("found a new entity, %lu\n", temp_entity);
                #endif
            }
          }
        }

        if (temp_counter > maximum)
        {
          maximum = temp_counter;
          memcpy(entities_string, prev_rel->dest_name, sizeof(String));
          strcat(entities_string, " ");
        }
        else if (temp_counter != 0 && temp_counter == maximum)
        {
          strcat(entities_string, prev_rel->dest_name);
          strcat(entities_string, " ");
        }

        if (maximum > 0)
        {
          strcat(report_string, relation->name);
          strcat(report_string, " ");
          strcat(report_string, entities_string);
          sprintf(tmp, "%d; ", maximum);
          strcat(report_string, tmp);
          i ++;
          #ifdef deb
            printf("maximum is > 0, added relation to report array.\n");
          #endif
        }
    }

  }
  #ifdef deb
    printf("finished phase one, printing...\n");
  #endif

  }

  printf("%s\n", report_string);
  return 1;
}

//debug functions---------------------------------------------------------------

#ifdef deb

void deb_print_entities(unsigned long entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE])
{
  int num;
  for (int i=0; i<ENTITY_TABLE_SIZE; i++)
  {
    num = 0;
    for (int j=0; j<COLLISION_BUFFER_SIZE; j++)
    {
      if (entity_table[i][j] != 0)
      {
        printf("%lu, ", entity_table[i][j]);
        num ++;
      }
    }
    if (num != 0)
      printf("\n");
  }
  printf("\n");
}

void deb_print_sub_relations(Relation* rel)
{
  int i;
  int rel_num = rel->sub_relation_number;
  SubRelation** array = (SubRelation**) rel->sub_rel_array;
  for(i=0; i<rel_num; i++)
  {
      if (array[i] != NULL)
      {
        printf("%d: %lu --%s--> %lu\n", i, array[i]->source_value, rel->name, array[i]->destination_value);
      }
      else
        printf("%d: NULLED\n", i);
  }
}

void deb_print_relations(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], int number_of_relations)
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
  unsigned long entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE] = {0};
  unsigned long deleted_entities_array[1024];
  int del_ent_num = 0;
  Relation* relations_buffer[RELATIONS_BUFFER_SIZE];
  //aux variables
  String argument0;
  String argument1;
  String argument2;
  LongString input_string;
  LongString prev_command = "";
  int report_needed = 0; // 0 = report has not changed, 1 = report has changed.

  int number_of_relations = 0;

  #ifdef deb
    int command_counter = 0;
  #endif

  int opcode, prev_opcode = 5;
  while (1)
  {
    //get input
    fgets(input_string, sizeof(LongString), stdin);

    //generate opcode
    if (input_string[0] == 'a') //addent or addrel
    {
      if (input_string[3] == 'e') //addent
        opcode =  0;
      else //addrel
        opcode =  1;
    }
    else if (input_string[0] == 'd') //delent or delrel
    {
      if (input_string[3] == 'e') //delent
        opcode =  2;
      else //delrel
        opcode =  3;
    }
    else if (input_string[0] == 'r') //report
      opcode =  4;
    else
      opcode =  5; //end

    if (opcode != 4)
    {
      if (prev_opcode == opcode)
      {
        if (strcmp(prev_command+6, input_string+6) == 0)
          opcode = 6;
        else
        {
          prev_opcode = opcode;
          memcpy(prev_command, input_string, sizeof(LongString));
        }
      }
      else
      {
        prev_opcode = opcode;
        memcpy(prev_command, input_string, sizeof(LongString));
      }
    }

    #ifdef deb
      printf("\nCOMMAND %d: %s\n", command_counter, input_string);
      command_counter ++;
    #endif

    switch (opcode)
    {
      case 0: //addent
      {
        get_argument_no_return(input_string, argument0, 8);
        handle_entity_creation(entity_table, entity_hash_function(argument0));
        #ifdef deb
          deb_print_entities(entity_table);
          //deb_print_relations(relations_buffer);
        #endif
        break;
      }
      case 1: //addrel
      {
        opcode = get_argument(input_string, argument0, 8);
        opcode = get_argument(input_string, argument1, opcode+3);
        get_argument_no_return(input_string, argument2, opcode+3);
        int tmp = handle_relation_creation(entity_table, relations_buffer,  argument0, argument1, argument2, number_of_relations);
        if (tmp != 0)
        {
          if (tmp == 2)
            number_of_relations ++;
          report_needed = 1;
        }
        #ifdef deb
          //deb_print_entities(entity_table);
          deb_print_relations(relations_buffer, number_of_relations);
        #endif
        break;
      }
      case 2: //delent
      {
        get_argument_no_return(input_string, argument0, 8);
        unsigned long ent = delent_function(entity_table, entity_hash_function(argument0));
        if (ent != 0)
        {
          deleted_entities_array[del_ent_num] = ent;
          del_ent_num ++;
          report_needed = 1;
        }
        #ifdef deb
          deb_print_entities(entity_table);
          deb_print_relations(relations_buffer, number_of_relations);
        #endif
        break;
      }
      case 3: //delrel
      {
        opcode = get_argument(input_string, argument0, 8);
        opcode = get_argument(input_string, argument1, opcode+3);
        get_argument_no_return(input_string, argument2, opcode+3);
        if (delrel_function(entity_table, relations_buffer, argument0, argument1, argument2, number_of_relations) != 0)
          report_needed = 1;
        #ifdef deb
          //deb_print_entities(entity_table);
          deb_print_relations(relations_buffer, number_of_relations);
        #endif
        break;
      }
      case 4: //report
      {
        report_function(entity_table, relations_buffer, report_needed, deleted_entities_array, del_ent_num, number_of_relations);
        del_ent_num = 0;
        report_needed = 0;
        #ifdef deb
          //deb_print_entities(entity_table);
          deb_print_relations(relations_buffer, number_of_relations);
        #endif
        break;
      }
      case 5: //end
      {
        return 0;
        break;
      }
      case 6:
      {
        #ifdef deb
          printf("same command\n");
        #endif
        break;
      }
    }
  }
}
