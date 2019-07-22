#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTITY_TABLE_SIZE 312679
#define RELATIONS_TABLE_SIZE 13537
#define SUB_RELATIONS_ARRAY_SIZE 16384

/* add -Ddeb to gcc compiler options to compile in verbose debug mode */

//type definitions--------------------------------------------------------------

typedef char SuperLongString[1024];
typedef char LongString[160];
typedef char String[50];

typedef struct {
  String name;
  int hash_value;

  void* ordered_relation; //points to the ordered relation.
  void* table_next; //Relation*
  void* table_prev; //Relation*

  int sub_relation_number;
  void* sub_rel_array[SUB_RELATIONS_ARRAY_SIZE];
} Relation;

typedef struct {
  Relation* relation; //points to the relation in hash table
  int position; //position in the list
  void* next;  //OrdRel*
  void* prev;  //OrdRel*
} OrdRel; //for speed and redundancy

typedef struct {
  String name;
  int entity_id; //add 1 when entity changes.
  int valid;     // = 1 when valid, else = 0.
  void* table_next; //Entity*
} Entity;

typedef struct {
  Relation* relation;
  Entity* source;
  Entity* destination;
  int source_id;
  int destination_id;
  void* table_next; //SubRelation*
} SubRelation;  //the destination is the entity that contains it.

typedef struct {
  Entity* entity;
  int occurrences;
} Pop; //struct used during report

int number_of_entities = 0;
int first_entity_position = ENTITY_TABLE_SIZE;
int last_entity_position = 0;
Entity* entity_table[ENTITY_TABLE_SIZE];          // = table 0
Relation* relations_table[RELATIONS_TABLE_SIZE];  // = table 1

OrdRel* rel_list_head = NULL;
int number_of_relations = 0;

//------------------------------------------------------------------------------
//functions definitions---------------------------------------------------------

int hash_function(char* text, const int table_size)
{
  int value = 0;
  for (int i=0; text[i] != '\0'; i++)
  {
    value += text[i]*(i+1)+text[i];
  }
  value =  value % table_size;
  #ifdef deb
    printf("hash value: %d\n", value);
  #endif
  return value;
}
//data la grandezza della tabella e una stringa in input genera un valore
//nel range della grandezza della tablella.

int get_argument(LongString input_string, char* dest_string, int start_pos)
{
  char byte = input_string[start_pos];
  int string_length = 0;
  while (byte != ' ' && byte != '\n' && byte != '\0') //while the char is valid
  {
    dest_string[string_length] = byte;
    string_length ++;
    start_pos ++;
    byte = input_string[start_pos];
  }
  dest_string[string_length] = '\0';
  return start_pos+1;
}
//data in ingresso la stringa in stdin, la analizza e riempie la stringa
//dest_string (argomento) con la stringa trovata a partire da start_pos
//fino ad uno spazio.

Entity* entity_hash_table_linear_search(String name, int table_pos)
{
  Entity* rel = entity_table[table_pos];
  while (rel != NULL)
  {
    if (strcmp(name, rel->name) == 0)
    {
      #ifdef deb
        printf("linear search done, found relation\n");
      #endif
      return rel;
    }
    rel = (Entity*) rel->table_next;
  }
  #ifdef deb
    printf("linear search done, no result\n");
  #endif
  return 0;
}

Relation* relation_hash_table_linear_search(String name, int table_pos)
{
  Relation* rel = relations_table[table_pos];
  while (rel != NULL)
  {
    if (strcmp(name, rel->name) == 0)
    {
      #ifdef deb
        printf("linear search done, found relation\n");
      #endif
      return rel;
    }
    rel = (Relation*) rel->table_next;
  }
  return 0;
}

int check_relation_validity(SubRelation* sub_rel)
{
  Entity* src = sub_rel->source;
  Entity* dest = sub_rel->destination;
  if (src->valid == 1 && dest->valid == 1)
    if (src->entity_id == sub_rel->source_id && dest->entity_id == sub_rel->destination_id)
      return 1;
  return 0;
}
//check the validity of a relation (1 = valid, 0 = not valid).

Entity* get_entity(String name)
{
  int pos = hash_function(name, ENTITY_TABLE_SIZE);
  Entity* entity = entity_hash_table_linear_search(name, pos);
  return entity;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

Relation* get_relation(String name)
{
  int pos = hash_function(name, RELATIONS_TABLE_SIZE);
  Relation* entity = relation_hash_table_linear_search(name, pos);
  return entity;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return relation.

Entity* create_entity(String name, int table_pos)
{
  //create the entity in the hash table
  Entity* self;
  self = (Entity*) malloc(sizeof(Entity));
  strcpy(self->name, name);
  self->table_next = NULL;
  self->entity_id = 0;
  self->valid = 1;

  if (table_pos > last_entity_position)
    last_entity_position = table_pos;
  if (table_pos < first_entity_position)
    first_entity_position = table_pos;

  number_of_entities ++;
  return self;
}
//crea entità, assegna il nome e ritorna il puntatore.

void reallocate_entity(Entity* self, String name)
{
  strcpy(self->name, name);
  self->valid = 1;
  number_of_entities ++;
}

void deallocate_entity(Entity* self)
{
  self->entity_id ++;
  self->valid = 0;
  strcpy(self->name, "");
  number_of_entities --;
}

Relation* create_relation(String name, int hash)
{
  //create the relation to put in hash table
  Relation* self;
  self = (Relation*) malloc(sizeof(Relation));
  strcpy(self->name, name);
  self->hash_value = hash;
  self->table_next = NULL;
  self->table_prev = NULL;
  //create ordered list entry
  OrdRel* ord = (OrdRel*) malloc(sizeof(OrdRel));
  ord->relation = self;
  self->ordered_relation = ord;
  self->sub_relation_number = 0;
  OrdRel* pointer = rel_list_head;
  OrdRel* prev_pointer = NULL;
  //put entry in ordered list
  while (pointer != NULL)
  {
    if (strcmp(self->name, (pointer->relation)->name) < 0)
    {
      if (prev_pointer == NULL)
      {
        ord->next = rel_list_head;
        ord->prev = NULL;
        rel_list_head = ord;
        //ordered_relation_list_fixup(ord, 0);
        number_of_relations ++;
        return self;
      }
      prev_pointer->next = ord;
      ord->prev = prev_pointer;
      ord->next = pointer;
      //ordered_relation_list_fixup(ord, prev_pointer->position+1);
      number_of_relations ++;
      return self;
    }
    prev_pointer = pointer;
    pointer = (OrdRel*) pointer->next;
  }

  if (prev_pointer == NULL)
  {
    rel_list_head = ord;
    ord->prev = NULL;
    ord->position = 0;
  }
  else
  {
    prev_pointer->next = ord;
    ord->prev = prev_pointer;
    ord->position = number_of_relations;
  }
  ord->next = NULL;
  number_of_relations ++;
  return self;
}
//Create relation in hash table, create relation in ordered list and
//return the pointer to the relation in hash table.

void sub_rel_array_fixup(SubRelation** array, int start_pos, const int max, SubRelation* replacer)
{
  SubRelation* temp;
  for (int i=start_pos; i<max; i++)
  {
    temp = array[i];
    array[i] = replacer;
    replacer = temp;
  }
}
//given the pointer to the array to restore, the starting position, the
//end postion and the entity that replaces, this function restores
//the order in the array.

void sub_rel_array_fixup_delete(SubRelation** array, int start_pos, const int max)
{
  int i;
  for (i=start_pos; i<max; i++)
  {
    array[i] = array[i+1];
  }
  array[i] = NULL;
}
//given the pointer to the array to restore, the starting position
//and the end postion, this function restores the order in the array.

SubRelation* create_sub_relation(Entity* src, Relation* rel, Entity* dest)
{
  SubRelation* self = (SubRelation*) malloc(sizeof(SubRelation));
  self->source = src;
  self->relation = rel;
  self->destination = dest;
  self->source_id = src->entity_id;
  self->destination_id = dest->entity_id;
  return self;
}
//creates a sub relation and returns the pointer to that.

int relation_add_entities(Relation* rel, Entity* src, Entity* dest, int pos)
{
  //setup
  int i;
  int rel_num = rel->sub_relation_number;
  SubRelation* sub_rel;
  SubRelation* pointer;
  SubRelation** array =  (SubRelation**) rel->sub_rel_array;
  if (rel->sub_relation_number == 0)
  {
    sub_rel = create_sub_relation(src, rel, dest);
    #ifdef deb
      printf("created sub relation in first slot of relation\n");
    #endif
    array[0] = sub_rel;
    rel->sub_relation_number ++;
    return 1;
  }
  else
  {
    for (i=0; i<rel_num; i++)
    {
      pointer = array[i];
      #ifdef deb
        printf("pointer: %d\n", pointer);
        printf("sub-relation is valid\n");
      #endif

      if (pointer->source == src && pointer->destination == dest)
      {
          #ifdef deb
            printf("Relation already exists.\n");
          #endif
          return 0;
      }
      else
      {
            if (strcmp(dest->name, (pointer->destination)->name) < 0)
            {
              #ifdef deb
              printf("dest is smaller\n");
              #endif
              sub_rel = create_sub_relation(src, rel, dest);
              rel->sub_relation_number ++;
              sub_rel_array_fixup(array, i, rel_num+1, sub_rel);
              #ifdef deb
                printf("added sub relation in first slot of relation\n");
              #endif
              return 1;
            }
        }
      }
      rel->sub_relation_number ++;
      sub_rel = create_sub_relation(src, rel, dest);
      array[i] = sub_rel;
      #ifdef deb
        printf("created sub relation in last slot of relation\n");
      #endif
  }
  return 1;
}
//adds entities to the new/existing relation;
//return 0 if everything went okay, else 1.

int handle_entity_creation(String name)
{
  int pos = hash_function(name, ENTITY_TABLE_SIZE);
  if (entity_table[pos] == NULL)
  {
    entity_table[pos] = create_entity(name, pos);
    #ifdef deb
      printf("Created new entity in empty slot\n");
    #endif
    return 1;
  }
  else
  {
    Entity* entity = entity_table[pos];
    Entity* prev_entity;
    while (entity != NULL)
    {
      prev_entity = entity;
      if (entity->valid == 0)
      {
        reallocate_entity(entity, name);
        #ifdef deb
          printf("Reallocated entity\n");
        #endif
        return 1;
      }
      if (strcmp(name, entity->name) == 0)
      {
        #ifdef deb
          printf("Entity already exists\n");
        #endif
        return 0;
      }
      entity = (Entity*) entity->table_next;
    }
    #ifdef deb
      printf("Created new entity\n");
    #endif
    entity = create_entity(name, pos);
    prev_entity->table_next = entity;
    return 1;
  }
  #ifdef deb
    printf("Error\n");
  #endif
  return 0;
}
//handle entity creation, return the created entity if success, else 0

int handle_relation_creation(String name1, String name2, String name)
{
  int pos = hash_function(name, RELATIONS_TABLE_SIZE);
  Entity* e1 = get_entity(name1);
  Entity* e2 = get_entity(name2);

  if (e1 == 0 || e2 == 0) //check if entities exist.
  {
    #ifdef deb
      printf("one of the entities does not exist\n");
    #endif
    return 0;
  }

  if (relations_table[pos] == NULL)
  {
    #ifdef deb
      printf("Created new relation in empty table slot\n");
    #endif
    relations_table[pos] = create_relation(name, pos);
    relation_add_entities(relations_table[pos], e1, e2, pos);
    return 1;
  }
  else
  {
    Relation* rel = relations_table[pos];
    Relation* prev_rel;
    #ifdef deb
      printf("first relation slot isn't empty\n");
    #endif
    while (rel != NULL)
    {
      #ifdef deb
        printf("rel: %s\n", rel->name);
      #endif
      prev_rel = rel;
      if (strcmp(name, rel->name) == 0)
      {
        return relation_add_entities(rel, e1, e2, pos);
      }
      rel = (Relation*) rel->table_next;
    }
    rel = create_relation(name, pos);
    prev_rel->table_next = rel;
    rel->table_prev = prev_rel;
    #ifdef deb
      printf("Created new relation at list end\n");
    #endif
    return relation_add_entities(rel, e1, e2, pos);
  }
  #ifdef deb
    printf("Error\n");
  #endif
  return 0;
}
//handle relation creation, return 0 if nothing was created, the relation
//created or modified otherwise.

int delent_function(String name)
{
  Entity* entity = get_entity(name);
  if (entity == 0)
  {
    #ifdef deb
      printf("invalid entity.\n");
    #endif
    return 0;
  }
  deallocate_entity(entity);
  #ifdef deb
    printf("deallocated entity at %x\n", entity);
  #endif
  return 1;
}
//deallocates an entity; return 1 if succes, else 0

int delrel_function(String name_source, String name_dest, String rel_name)
{
  Relation* rel = get_relation(rel_name);
  Entity* src = get_entity(name_source);
  Entity* dest = get_entity(name_dest);
  SubRelation* sub_rel;
  int i, rel_num;
  SubRelation** array;

  if (src == 0 || dest == 0 || rel == 0) //check if entities and relation exist.
  {
    #ifdef deb
      printf("relation does not exist.\n");
    #endif
    return 0;
  }

  //delete sub relation from relation list in relation
  rel_num = rel->sub_relation_number;
  array = (SubRelation**) rel->sub_rel_array;
  for (i=0; i<rel_num; i++)
  {
    sub_rel = array[i];
    if (sub_rel->source == src && sub_rel->destination == dest)
    {
        if (i == 0)
        {
          free(sub_rel);
          rel->sub_relation_number --;
          sub_rel_array_fixup_delete(array, 0, rel->sub_relation_number);
          #ifdef deb
            printf("deleted relation in middle of list.\n");
          #endif
        }
        else
        {
          free(sub_rel);
          rel->sub_relation_number --;
          sub_rel_array_fixup_delete(array, i, rel->sub_relation_number);
          #ifdef deb
            printf("deleted relation at the start of the list.\n");
          #endif
        }
        return 1;
    }
  }
  #ifdef deb
    printf("relation does not exist (LATE)\n");
  #endif
  return 0;
}
//deletes a relation; return 1 if success, else 0

OrdRel* delete_unused_relation(Relation* rel)
{
  //setup pointers
  Relation* rel_prev = (Relation*) rel->table_prev;
  Relation* rel_next = (Relation*) rel->table_next;
  OrdRel* ord_rel = (OrdRel*) rel->ordered_relation;
  OrdRel* ord_rel_prev = (OrdRel*) ord_rel->prev;
  OrdRel* ord_rel_next = (OrdRel*) ord_rel->next;
  //delete the relation
  if (rel_prev == NULL)
    relations_table[rel->hash_value] = rel_next;
  else
    rel_prev->table_next = rel_next;
  free(rel);
  //delete the ordered relation
  if (ord_rel_prev == NULL)
    rel_list_head = ord_rel_next;
  else
    ord_rel_prev->next = ord_rel_next;
  free(ord_rel);
  //decrease counter
  number_of_relations --;
  return ord_rel_next;
}
//if it detects that a relation has 0 instances, it deletes it;
//called from report_function.

int report_function(int update)
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
    strcpy(report_string, "");

    //declarations of data structures used
    int ent_num = number_of_entities + 1;
    Pop entities_array[number_of_relations][ent_num];
    Relation* relations_array[number_of_relations+1];
    int maximums[number_of_relations];

    //aux variables
    OrdRel* pointer = rel_list_head;
    Entity* prev_entity;
    Entity* temp_entity;
    Relation* relation;
    SubRelation* sub_rel;
    SubRelation** array;
    char tmp_string[20];
    int i = 0, j, k, temp_counter = 0;
    int maximum;
    int rel_num;

  //get maximums and load entities in 2d array
  while (pointer != NULL)
  {
      relation = pointer->relation;
      prev_entity = NULL;
      rel_num = relation->sub_relation_number;
      if (rel_num > 0)
      {
        #ifdef deb
          printf("relation %s is valid, accessing sub-relations\n", relation->name);
        #endif
        maximums[i] = j = temp_counter = 0;
        array = (SubRelation**) relation->sub_rel_array;
        for (k=0; k<rel_num; k++)
        {
          sub_rel = array[k];
          if (check_relation_validity(sub_rel) == 1)
          {
            temp_entity = sub_rel->destination;
            #ifdef deb
              printf("prev: %s, current: %s\n", prev_entity->name, temp_entity->name);
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
              if (prev_entity != NULL)
              {
                if (temp_counter > maximums[i])
                  maximums[i] = temp_counter;
                entities_array[i][j].entity = prev_entity;
                entities_array[i][j].occurrences = temp_counter;
                #ifdef deb
                  printf("%d|%d: %s, %d;\n", i, j, prev_entity->name, temp_counter);
                #endif
                j ++;
                prev_entity = temp_entity;
                temp_counter = 1;
                #ifdef deb
                  printf("found a new entity, %s\n", temp_entity->name);
                #endif
              }
              else
              {
                prev_entity = temp_entity;
                temp_counter = 1;
                #ifdef deb
                  printf("found first entity\n");
                #endif
              }
            }
          }
          else
          {
            free(sub_rel);
            if (k == 0)
            {
              relation->sub_relation_number --;
              rel_num --;
              sub_rel_array_fixup_delete(array, 0, rel_num);
              #ifdef deb
                printf("deleting first element of the list\n");
              #endif
            }
            else
            {
              relation->sub_relation_number --;
              rel_num --;
              sub_rel_array_fixup_delete(array, k, rel_num);
            }
            k --;
          }
        }

        if (temp_counter > maximums[i])
          maximums[i] = temp_counter;
        entities_array[i][j].entity = prev_entity;
        entities_array[i][j].occurrences = temp_counter;
        entities_array[i][j+1].occurrences = 0;

        if (maximums[i] > 0)
        {
          relations_array[i] = relation;
          i ++;
          pointer = (OrdRel*) pointer->next;
          #ifdef deb
            printf("maximum is > 0, added relation to report array.\n");
          #endif
        }
        else
        {
          #ifdef deb
            printf("relation %s is unused, deleting(LATE)\n", relation->name);
          #endif
          pointer = delete_unused_relation(relation);
        }
    }
    else
    {
      #ifdef deb
        printf("relation %s is unused, deleting\n", relation->name);
      #endif
      pointer = delete_unused_relation(relation);
    }
  }

  relations_array[i] = NULL;

  #ifdef deb
    printf("finished phase one, printing...\n");
  #endif

  if (relations_array[0] == NULL)
  {
    #ifdef deb
      printf("(LATE)\n");
    #endif
    printf("none\n");
    return 0;
  }

  for (i=0; relations_array[i] != NULL; i++)
  {
      maximum = maximums[i];
      strcat(report_string, relations_array[i]->name);
      strcat(report_string, " ");
      for (j=0; j<number_of_entities; j++)
      {
        if (entities_array[i][j].occurrences == 0)
          break;
        if (entities_array[i][j].occurrences == maximum)
        {
          strcat(report_string, (entities_array[i][j].entity)->name);
          strcat(report_string, " ");
        }
      }
      sprintf(tmp_string, "%d; ", maximum);
      strcat(report_string, tmp_string);
  }

  }

  printf("%s\n", report_string);
  return 1;
}

//debug functions---------------------------------------------------------------

#ifdef deb

void deb_print_entities()
{
  Entity* entity;
  for (int i=0; i<ENTITY_TABLE_SIZE; i++)
  {
    if (entity_table[i] != NULL)
    {
        entity = entity_table[i];
        printf("%d: ", i);
        while (entity != NULL)
        {
          if (entity->valid == 1)
            printf("%s [id = %d], ", entity->name, entity->entity_id);
          else
            printf("(deallocated) [id = %d], ", entity->entity_id);
          entity = (Entity*) entity->table_next;
        }
        printf("\n");
    }
  }
}

void deb_print_sub_relations(Relation* rel)
{
  int i;
  int rel_num = rel->sub_relation_number;
  SubRelation** array = rel->sub_rel_array;
  for(i=0; i<rel_num; i++)
  {
      Entity* src = array[i]->source;
      Entity* dest = array[i]->destination;
      printf("%s --%s--> %s\n", src->name, rel->name, dest->name);
  }
}

void deb_print_relations()
{
  OrdRel* rel = rel_list_head;
  while (rel != NULL)
  {
    deb_print_sub_relations(rel->relation);
    rel = (OrdRel*) rel->next;
  }
}

void deb_print_ordered_relations()
{
  OrdRel* pointer = rel_list_head;
  while(pointer != NULL)
  {
    printf("%d: %s\n", pointer->position, (pointer->relation)->name);
    pointer = (OrdRel*) pointer->next;
  }
}

void deb_print_all()
{
  deb_print_entities();
  deb_print_relations();
  deb_print_ordered_relations();
}

#endif

//------------------------------------------------------------------------------

int generate_opcode(LongString input_string)
{
  if (input_string[0] == 'a') //addent or addrel
  {
    if (input_string[3] == 'e') //addent
      return 0;
    else //addrel
      return 1;
  }
  else if (input_string[0] == 'd') //delent or delrel
  {
    if (input_string[3] == 'e') //delent
      return 2;
    else //delrel
      return 3;
  }
  else if (input_string[0] == 'r') //report
    return 4;

  return 5; //end
}
//check the first 6 characters of the input string and generate
//the correct opcode.

//------------------------------------------------------------------------------
int main() //main program
{
  String argument0;
  String argument1;
  String argument2;
  LongString input_string;
  int report_needed = 1; // 0 = report has not changed, 1 = report has changed.
  #ifdef deb
    int command_counter = 0;
  #endif

  int opcode;
  while (1)
  {
    fgets(input_string, 160, stdin);
    opcode = generate_opcode(input_string);

    #ifdef deb
      printf("\nCOMMAND %d: %s\n", command_counter, input_string);
      command_counter ++;
    #endif

    switch (opcode)
    {
      case 0: //addent
      {
        get_argument(input_string, argument0, 7);
        handle_entity_creation(argument0);
        #ifdef deb
          deb_print_entities();
          deb_print_relations();
        #endif
        break;
      }
      case 1: //addrel
      {
        opcode = get_argument(input_string, argument0, 7);
        opcode = get_argument(input_string, argument1, opcode);
        get_argument(input_string, argument2, opcode);
        if (handle_relation_creation(argument0, argument1, argument2) != 0)
          report_needed = 1;
        #ifdef deb
          deb_print_relations();
        #endif
        break;
      }
      case 2: //delent
      {
        get_argument(input_string, argument0, 7);
        if (delent_function(argument0) != 0)
          report_needed = 1;
        #ifdef deb
          deb_print_entities();
          deb_print_relations();
        #endif
        break;
      }
      case 3: //delrel
      {
        opcode = get_argument(input_string, argument0, 7);
        opcode = get_argument(input_string, argument1, opcode);
        get_argument(input_string, argument2, opcode);
        if (delrel_function(argument0, argument1, argument2) != 0)
          report_needed = 1;
        #ifdef deb
          deb_print_relations();
        #endif
        break;
      }
      case 4: //report
      {
        if (report_function(report_needed) != 0)
          report_needed = 0;
        #ifdef deb
          deb_print_relations();
        #endif
        break;
      }
      case 5: //end
      {
        return 0;
        break;
      }
    }
  }
}
