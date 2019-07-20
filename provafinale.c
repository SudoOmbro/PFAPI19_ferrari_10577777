#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUB_REL_TABLE_SIZE 499
#define ENTITY_TABLE_SIZE 91711
#define RELATIONS_TABLE_SIZE 353

/* add -Ddeb to gcc compiler options to compile in verbose debug mode */

//type definitions--------------------------------------------------------------

typedef char LongString[160];
typedef char String[50];

typedef struct {
  String name;
  int hash_value;
  void* first_sub_relation;
  void* ordered_relation; //points to the ordered relation.
  void* table_next; //Relation*
  void* table_prev; //Relation*
} Relation;

typedef struct {
  Relation* relation; //points to the relation in hash table
  int position; //position in the list
  void* next;  //OrdRel*
  void* prev;  //OrdRel*
} OrdRel; //for speed and redundancy

typedef struct {
  String name;

  int sub_relations_number;
  void* sub_relations_out[SUB_REL_TABLE_SIZE];  //relations going out
  void* sub_relations_in[SUB_REL_TABLE_SIZE];  //relations going out
  int first_sub_rel;
  int last_sub_rel;

  int entity_id; //add 1 when entity changes.
  int valid;     // = 1 when valid, else = 0.
  void* table_next; //Entity*
} Entity;

typedef struct {
  Relation* relation;
  Entity* source;
  Entity* destination;
  int source_id;
  int valid;
  void* table_next; //SubRelation*
} SubRelation;  //the destination is the entity that contains it.

typedef struct {
  SubRelation* rel;
  void* next;
} SubRelation_pointer;

typedef struct {
  Entity* entity;
  int occurrences;
  void* next;
} PopEntity; //struct used during report

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
  char byte = text[0];
  for (int i=0; byte != '\0'; i++)
  {
    value += byte*(i+1)+byte;
    byte = text[i];
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

void* global_hash_table_linear_search(String name, int table_pos, const int table)
{
  switch(table)
  {
    case 0: //entity table
    {
      Entity* entity = entity_table[table_pos];
      while (entity != NULL)
      {
        if (strcmp(name, entity->name) == 0)
        {
          #ifdef deb
            printf("linear search done, found entity\n");
          #endif
          return entity;
        }
        entity = (Entity*) entity->table_next;
      }
      break;
    }
    case 1: //relations table
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
      break;
    }
  }
  #ifdef deb
    printf("linear search done, no result\n");
  #endif
  return 0;
}
//returns 0 if nothing is found, else it returns the pointer
//of the wanted thing

Entity* get_entity(String name)
{
  int pos = hash_function(name, ENTITY_TABLE_SIZE);
  Entity* entity = (Entity*) global_hash_table_linear_search(name, pos, 0);
  return entity;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

Entity* get_entity_prev(String name, int pos)
{
  Entity* entity = (Entity*) entity_table[pos];
  Entity* prev_entity = entity;
  while(entity != NULL)
  {
    entity = (Entity*) entity->table_next;
    if (strcmp(name, entity->name) == 0)
    {
      #ifdef deb
        printf("found entity to delete in linear search.\n");
      #endif
      return prev_entity;
    }
    prev_entity = entity;
  }
  #ifdef deb
    printf("no entry found to delete.\n");
  #endif
  return 0;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

Relation* get_relation(String name)
{
  int pos = hash_function(name, RELATIONS_TABLE_SIZE);
  Relation* entity = (Relation*) global_hash_table_linear_search(name, pos, 1);
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
  self->first_sub_rel = SUB_REL_TABLE_SIZE;
  self->last_sub_rel = 0;

  if (table_pos > last_entity_position)
    last_entity_position = table_pos;
  if (table_pos < first_entity_position)
    first_entity_position = table_pos;

  return self;
}
//crea entità, assegna il nome e ritorna il puntatore.

void reallocate_entity(Entity* self, String name)
{
  strcpy(self->name, name);
  self->valid = 1;
}

void deallocate_entity(Entity* self)
{
  self->entity_id ++;
  self->valid = 0;
  self->first_sub_rel = SUB_REL_TABLE_SIZE;
  self->last_sub_rel = 0;
  strcpy(self->name, "");
}

void ordered_relation_list_fixup(OrdRel* start_rel, int pos)
{
  OrdRel* rel = start_rel;
  while (rel != NULL)
  {
    rel->position = pos;
    pos ++;
    rel = (OrdRel*) rel->next;
  }
}
//fixes the order of the ordered list from the indicated starting position

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
  self->first_sub_relation = NULL;
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

SubRelation* create_sub_relation(Entity* src, Relation* rel, Entity* dest)
{
  SubRelation* self = (SubRelation*) malloc(sizeof(SubRelation));
  self->source = src;
  self->relation = rel;
  self->destination = dest;
  self->source_id = src->entity_id;
  self->valid = 1;
  self->table_next = NULL;
  return self;
}

void set_sub_relation_first_and_last(Entity* entity, int pos)
{
  if (entity->first_sub_rel > pos)
    entity->first_sub_rel = pos;
  if (entity->last_sub_rel < pos)
    entity->last_sub_rel = pos;
}

SubRelation_pointer* add_sub_rel_to_pointer(SubRelation* rel)
{
  SubRelation_pointer* sub = (SubRelation_pointer*) malloc(sizeof(SubRelation_pointer));
  sub->rel = rel;
  sub->next = NULL;
  return sub;
}

SubRelation* delete_invalid_relation(SubRelation* rel)
{
  #ifdef deb
    printf("deleting invalid relation: %x\n", rel);
  #endif
  SubRelation* next = (SubRelation*) rel->table_next;
  free(rel);
  return next;
}

int relation_add_entities(Relation* rel, Entity* src, Entity* dest, int pos)
{
  //setup
  SubRelation* sub_rel;
  //add sub relation to relation
  SubRelation* rel_start = (SubRelation*) rel->first_sub_relation;
  if (rel_start == NULL)
  {
    sub_rel = create_sub_relation(src, rel, dest);
    #ifdef deb
      printf("created sub relation in first slot of relation\n");
    #endif
    rel->first_sub_relation = sub_rel;
  }
  else
  {
    SubRelation* pointer = rel_start;
    SubRelation* prev_pointer = NULL;
    SubRelation* destination = NULL;
    SubRelation* prev_dest = NULL;
    while(pointer != NULL)
    {
      #ifdef deb
        printf("pointer: %d\n", pointer);
      #endif
      if (pointer->valid == 1)
      {
        #ifdef deb
          printf("sub-relation is valid\n");
        #endif
        if (pointer->source == src && pointer->destination == dest)
        {
          #ifdef deb
            printf("Relation already exists.\n");
            #endif
            return 1;
          }
          else if (destination == NULL)
          {
            if (strcmp(dest->name, (pointer->destination)->name) < 0)
            {
              #ifdef deb
              printf("dest is smaller\n");
              #endif
              destination = pointer;
              prev_dest = prev_pointer;
              break;
            }
          }
          #ifdef deb
            printf("dest is bigger\n");
          #endif
          prev_pointer = pointer;
          pointer = (SubRelation*) pointer->table_next;
        }
        else
        {
          #ifdef deb
            printf("sub-relation is invalid\n");
          #endif
          pointer = delete_invalid_relation(pointer);
          #ifdef deb
            printf("new pointer: %x\n", pointer);
          #endif
          if (prev_pointer != NULL)
            prev_pointer->table_next = pointer;
          else
            rel->first_sub_relation = pointer;
        }
      }
      sub_rel = create_sub_relation(src, rel, dest);

      if (destination == NULL)
      {
        prev_pointer->table_next = sub_rel;
        #ifdef deb
          printf("created sub relation in last slot of relation\n");
        #endif
      }
      else
      {
        if (prev_dest == NULL)
        {
          sub_rel->table_next = rel_start;
          rel->first_sub_relation = sub_rel;
          #ifdef deb
            printf("added sub relation in first slot of relation\n");
          #endif
        }
        else
        {
          sub_rel->table_next = destination;
          prev_dest->table_next = sub_rel;
          #ifdef deb
            printf("added sub relation in middle slot of relation\n");
          #endif
        }
      }
  }

  //add relation to in relations of the source
  SubRelation_pointer** start = (SubRelation_pointer**) (dest->sub_relations_in+pos);
  SubRelation_pointer* sub_rel_p;
  SubRelation_pointer* prev_rel_p;
  if (*start == NULL)
  {
    *start = add_sub_rel_to_pointer(sub_rel);
    #ifdef deb
      printf("Added entries to relation, first place\n");
    #endif
  }
  else
  {
    sub_rel_p = *start;
    while(sub_rel_p != NULL)
    {
      prev_rel_p = sub_rel_p;
      sub_rel_p = (SubRelation_pointer*) sub_rel_p->next;
    }
    sub_rel_p = add_sub_rel_to_pointer(sub_rel);
    prev_rel_p->next = sub_rel_p;
  }

  //add relation to source entity
  start = (SubRelation_pointer**) (src->sub_relations_out+pos);
  if (*start == NULL)
  {
    *start = add_sub_rel_to_pointer(sub_rel);
    #ifdef deb
      printf("Added entries to relation, first place\n");
    #endif
  }
  else
  {
    sub_rel_p = *start;
    while(sub_rel_p != NULL)
    {
      prev_rel_p = sub_rel_p;
      sub_rel_p = (SubRelation_pointer*) sub_rel_p->next;
    }
    sub_rel_p = add_sub_rel_to_pointer(sub_rel);
    prev_rel_p->next = sub_rel_p;
  }

  set_sub_relation_first_and_last(dest, pos);
  set_sub_relation_first_and_last(src, pos);
  #ifdef deb
    printf("Added entries to relation\n");
  #endif

  return 0;
}

//adds entities to the new/existing relation;
//return 0 if everything went okay, else 1.

Entity* handle_entity_creation(String name)
{
  int pos = hash_function(name, ENTITY_TABLE_SIZE);
  if (entity_table[pos] == NULL)
  {
    entity_table[pos] = create_entity(name, pos);
    #ifdef deb
      printf("Created new entity in empty slot\n");
    #endif
    return entity_table[pos];
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
        return entity;
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
    return entity;
  }
  #ifdef deb
    printf("Error\n");
  #endif
  return 0;
}
//handle entity creation, return the created entity if success, else 0

Relation* handle_relation_creation(String name1, String name2, String name)
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
    return relations_table[pos];
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
        relation_add_entities(rel, e1, e2, pos);
        return rel;
      }
      rel = (Relation*) rel->table_next;
    }
    rel = create_relation(name, pos);
    prev_rel->table_next = rel;
    rel->table_prev = prev_rel;
    #ifdef deb
      printf("Created new relation at list end\n");
    #endif
    relation_add_entities(rel, e1, e2, pos);
    return rel;
  }
  #ifdef deb
    printf("Error\n");
  #endif
  return 0;
}
//handle relation creation, return 0 if nothing was created, the relation
//created or modified otherwise.

SubRelation* delete_sub_relation(SubRelation* sub_rel)
{
  SubRelation* next = (SubRelation*) sub_rel->table_next;
  free(sub_rel);
  return next;
}
//deletes the specified sub relation, return the next pointer.
//if there is no next pointer, return NULL.

SubRelation_pointer* delete_sub_relation_pointer(SubRelation_pointer* rel)
{
  SubRelation_pointer* next = (SubRelation_pointer*) rel->next;
  (rel->rel)->valid = 0;
  #ifdef deb
    printf("deleted pointer to %s.\n", ((rel->rel)->relation)->name);
  #endif
  free(rel);
  return next;
}

void delete_relation_stack(Entity* entity)
{
  SubRelation_pointer** in_start;
  SubRelation_pointer** out_start;
  for(int i=entity->first_sub_rel; i<SUB_REL_TABLE_SIZE; i++)
  {
    in_start = (SubRelation_pointer**) (entity->sub_relations_in+i);
    out_start = (SubRelation_pointer**) (entity->sub_relations_out+i);
    while (*in_start != NULL)
    {
      *in_start = delete_sub_relation_pointer(*in_start);
      #ifdef deb
        printf("deleted in relation\n");
      #endif
    }
    while (*out_start != NULL)
    {
      *out_start = delete_sub_relation_pointer(*out_start);
      #ifdef deb
        printf("deleted in relation\n");
      #endif
    }
  }
}
//delete all the relations towards the deleted entity.

int delent_function(String name)
{
  Entity* entity = get_entity(name);
  if (entity == 0)
  {
    #ifdef deb
      printf("invalid entity.\n");
    #endif
    return 1;
  }
  delete_relation_stack(entity);
  deallocate_entity(entity);
  #ifdef deb
    printf("deallocated entity at %x\n", entity);
  #endif
  return 0;
}
//deallocates an entity; return 0 if succes, else 1

int delrel_function(String name_source, String name_dest, String rel_name)
{
  Relation* rel = get_relation(rel_name);
  Entity* e1 = get_entity(name_source);
  Entity* e_dest = get_entity(name_dest);

  if (e1 == 0 || e_dest == 0 || rel == 0) //check if entities and relation exist.
  {
    #ifdef deb
      printf("one of the entities does not exist\n");
    #endif
    return 0;
  }

  int pos = hash_function(rel_name, SUB_REL_TABLE_SIZE);
  SubRelation_pointer** start = (SubRelation_pointer**) (e_dest->sub_relations_in+pos);
  //delete relation in destination
  if (*start != NULL)
  {
    SubRelation_pointer* sub_rel = *start;
    SubRelation_pointer* prev_rel = NULL;
    while(sub_rel != NULL)
    {
      SubRelation* relation = sub_rel->rel;
      if (relation->source == e1 && relation->relation == rel)
      {
        if (prev_rel == NULL)
        {
          *start = delete_sub_relation_pointer(sub_rel);
          break;
        }
        else
        {
          prev_rel->next = delete_sub_relation_pointer(sub_rel);
          break;
        }
      }
      prev_rel = sub_rel;
      sub_rel = (SubRelation_pointer*) sub_rel->next;
    }
  }
  start = (SubRelation_pointer**) (e1->sub_relations_out+pos);
  //delete out relation in source
  if (*start != NULL)
  {
    SubRelation_pointer* sub_rel = *start;
    SubRelation_pointer* prev_rel = NULL;
    while(sub_rel != NULL)
    {
      SubRelation* relation = sub_rel->rel;
      if (relation->destination == e_dest && relation->relation == rel)
      {
        if (prev_rel == NULL)
        {
          *start = delete_sub_relation_pointer(sub_rel);
          break;
        }
        else
        {
          prev_rel->next = delete_sub_relation_pointer(sub_rel);
          break;
        }
      }
      prev_rel = sub_rel;
      sub_rel = (SubRelation_pointer*) sub_rel->next;
    }
  }
  //delete sub relation from relation list in relation
  SubRelation* sub_rel = (SubRelation*) rel->first_sub_relation;
  SubRelation* sub_rel_prev = NULL;
  while (sub_rel != NULL)
  {
    if (sub_rel->valid == 1)
    {
      if (sub_rel->source == e1 && sub_rel->destination == e_dest)
      {
        if (sub_rel_prev != NULL)
        {
          sub_rel = delete_sub_relation(sub_rel);
          sub_rel_prev->table_next = sub_rel;
          #ifdef deb
            printf("deleted relation in middle of list.\n");
          #endif
        }
        else
        {
          sub_rel = delete_sub_relation(sub_rel);
          rel->first_sub_relation = sub_rel;
          #ifdef deb
            printf("deleted relation at the start of the list.\n");
          #endif
        }
        return pos;
      }
      sub_rel_prev = sub_rel;
      sub_rel = (SubRelation*) sub_rel->table_next;
    }
    else
    {
      sub_rel = delete_invalid_relation(sub_rel);
      #ifdef deb
        printf("deleted an invalid relation.\n");
      #endif
    }
  }

  #ifdef deb
    printf("no relation found.\n");
  #endif
  return pos;
}
//return the postion of the relation in the sub array for the specific relation

SubRelation* delete_obsolete_relation_old(int i, SubRelation* prev_rel, Entity* entity, SubRelation* sub_rel)
{
  if (prev_rel == NULL)
  {
    *(entity->sub_relations_in+i) = delete_sub_relation(sub_rel);
    sub_rel = (SubRelation*) *(entity->sub_relations_in+i);
  }
  else
  {
    prev_rel->table_next = delete_sub_relation(sub_rel);
    sub_rel = (SubRelation*) prev_rel->table_next;
  }
  return sub_rel;
}
//DELETE

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

PopEntity* create_pop_entity(Entity* entity)
{
  PopEntity* pop = (PopEntity*) malloc(sizeof(PopEntity));
  pop->entity = entity;
  pop->occurrences = 1;
  pop->next = NULL;
  #ifdef deb
    printf("created popular entity %s\n", entity->name);
  #endif
  return pop;
}

#ifdef aaa
int report_function_old()
{
  //if there are no relations, do nothing
  if (rel_list_head == NULL)
  {
    printf("none\n");
    return 0;
  }

  Entity* entity;
  Entity* src;
  Relation* relation;
  SubRelation* prev_rel;
  SubRelation* sub_rel;

  int i, j, position;
  int max_value_temp[number_of_relations];
  PopEntity* popular_entities_temp[number_of_relations];
  int max_value[number_of_relations];
  PopEntity* popular_entities[number_of_relations];
  Relation* relations[number_of_relations];

  for (i=0; i<number_of_relations; i++)
    popular_entities_temp[i] = NULL;

  //set all the maximums to 0
  for (i=0; i<number_of_relations; i++)
    max_value_temp[i] = 0;
    #ifdef deb
      printf("starting phase one.\n");
    #endif
  int cycle_end = last_entity_position + 1;
  for (j=first_entity_position; j<cycle_end; j++)
  {
    //load the entities popularity for each relation in the temp array
    if (entity_table[j] != NULL)
    {
      entity = entity_table[j];
      while (entity != NULL)
      {
        #ifdef deb
          printf("found an entity, %s\n", entity->name);
        #endif
        if (entity->valid == 1)
        {
          #ifdef deb
            printf("the entity is valid\n");
          #endif
          int last_relation = entity->last_sub_rel + 1;
          for (int i=entity->first_sub_rel; i<last_relation; i++)
          {
            sub_rel = (SubRelation*) *(entity->sub_relations_in+i);
            prev_rel = NULL;
            while (sub_rel != NULL)
            {
              #ifdef deb
                printf("found a relation\n"()());
              #endif
              src = sub_rel->source;
              relation = sub_rel->relation;
              if (src->valid == 1)
              {
                if (sub_rel->source_id == src->entity_id)
                {
                  position = ((OrdRel*)(relation->ordered_relation))->position;
                  #ifdef deb
                    printf("the relation is valid, pos: %d\n", position);
                  #endif
                  if (popular_entities_temp[position] == NULL)
                  {
                    #ifdef deb
                      printf("first position is free\n");
                    #endif
                    popular_entities_temp[position] = create_pop_entity(entity);
                    max_value_temp[position] = 1;
                    #ifdef deb
                      printf("max[%d] = %d\n", position, max_value_temp[position]);
                      printf("entity added: %s\n", (popular_entities_temp[position]->entity)->name);
                    #endif
                  }
                  else
                  {
                    #ifdef deb
                      printf("first position is not free\n");
                    #endif
                    PopEntity* pe = popular_entities_temp[position];
                    PopEntity* pe_prev = NULL;
                    PopEntity* destination;
                    while(pe != NULL)
                    {
                      #ifdef deb
                        printf("%s, \n", (pe->entity)->name);
                      #endif
                      if (pe->entity == entity)
                      {
                        #ifdef deb
                          printf("same dest found, increasing max.\n");
                        #endif
                        pe->occurrences ++;
                        if (max_value_temp[position] < pe->occurrences)
                        {
                          max_value_temp[position] = pe->occurrences;
                          #ifdef deb
                            printf("entity found: %s\n", (pe->entity)->name);
                            printf("max[%d] = %d\n", position, max_value_temp[position]);
                          #endif
                        }
                        break;
                      }
                      else
                      {
                        #ifdef deb
                          printf("the entity has to be put in the table\n");
                        #endif
                        if (strcmp(entity->name, ((Entity*)(pe->entity))->name) < 0)
                        {
                          #ifdef deb
                            printf("the entity is inferior\n");
                          #endif
                          destination = pe_prev;
                          pe = NULL;
                        }
                        else
                        {
                          #ifdef deb
                            printf("the entity is superior\n");
                          #endif
                          destination = pe;
                          pe_prev = pe;
                          pe = (PopEntity*) pe->next;
                        }
                      }
                    }
                    if (pe == NULL)
                    {
                      //add a popular entity.
                      if (destination == NULL)
                      {
                        pe = popular_entities_temp[position];
                        popular_entities_temp[position] = create_pop_entity(entity);
                        popular_entities_temp[position]->next = pe;
                        #ifdef deb
                          printf("putting entity first\n");
                          char* start = (popular_entities_temp[position]->entity)->name;
                          char* next = (((PopEntity*)popular_entities_temp[position]->next)->entity)->name;
                          printf("start: %s, next: %s\n", start, next);
                        #endif
                      }
                      else
                      {
                        pe = (PopEntity*) destination->next;
                        destination->next = create_pop_entity(entity);
                        ((PopEntity*) destination->next)->next = pe;
                        #ifdef deb
                          printf("putting entity in the middle\n");
                          char* prev = (destination->entity)->name;
                          char* current = (((PopEntity*)destination->next)->entity)->name;
                          char* next = (pe->entity)->name;
                          printf("start: %s, current: %s, next: %s\n", prev, current, next);
                        #endif
                      }
                    }
                  }
                  prev_rel = sub_rel;
                  sub_rel = (SubRelation*) sub_rel->table_next;
                }
                else
                  sub_rel = delete_obsolete_relation_old(i, prev_rel, entity, sub_rel);
              }
              else
                sub_rel = delete_obsolete_relation_old(i, prev_rel, entity, sub_rel);
            }
          }
        }
        entity = (Entity*) entity->table_next;
      }
    }
  }
  #ifdef deb
    for (i=0; i<number_of_relations; i++)
    {
      printf("%d: ", i);
      if (popular_entities_temp[i] != NULL)
      {
        PopEntity* tmp = popular_entities_temp[i];
        while (tmp !=  NULL)
        {
          printf("%s, %d; ", (tmp->entity)->name, tmp->occurrences);
          tmp = tmp->next;
        }
      }
      printf("\n");
    }
    printf("phase one finished, starting phase two.\n");
  #endif
  OrdRel* ord_rel = rel_list_head;

  j = i = 0;

  while(ord_rel != NULL)
  {
    #ifdef deb
      printf("found relation\n");
    #endif
    //if the max value for a relation is zero, it gets eliminated.
    if (max_value_temp[i] == 0)
    {
      #ifdef deb
        printf("deleting unused relation\n");
      #endif
      ord_rel = delete_unused_relation(ord_rel->relation);
    }
    else
    {
      max_value[j] = max_value_temp[i];
      int max_val = max_value[j];
      PopEntity* pop_ent = popular_entities_temp[i];
      PopEntity* prev_ent = NULL;
      while (pop_ent != NULL)
      {
        if (pop_ent->occurrences < max_val)
        {
          PopEntity* temp;
          if (prev_ent == NULL)
          {
            temp = (PopEntity*) popular_entities_temp[i]->next;
            free(popular_entities_temp[i]);
            popular_entities_temp[i] = temp;
          }
          else
          {
            temp = (PopEntity*) pop_ent->next;
            free(pop_ent);
            prev_ent->next = temp;
          }
          pop_ent = temp;
        }
        else
        {
          prev_ent = pop_ent;
          pop_ent = (PopEntity*) pop_ent -> next;
        }
      }
      popular_entities[j] = popular_entities_temp[i];
      relations[j] = ord_rel->relation;
      ord_rel = (OrdRel*) ord_rel->next;
      j ++;
    }
    i ++;
  }
  #ifdef deb
    for (i=0; i<number_of_relations; i++)
    {
      printf("%d: ", i);
      if (popular_entities[i] != NULL)
      {
        PopEntity* tmp = popular_entities[i];
        while (tmp !=  NULL)
        {
          printf("%s, %d; ", (tmp->entity)->name, tmp->occurrences);
          tmp = tmp->next;
        }
      }
      printf("\n");
    }
    printf("phase two finished, starting phase three.\n");
  #endif
  ordered_relation_list_fixup(rel_list_head, 0);
  PopEntity* pop_entity;
  for(i=0; i<number_of_relations; i++)
  {
    //print required stuff
    printf("%s ", relations[i]->name);
    pop_entity = popular_entities[i];
    PopEntity* pe;
    while (pop_entity != NULL)
    {
      printf("%s ", (pop_entity->entity)->name);
      pe = (PopEntity*) pop_entity->next;
      free(pop_entity);
      pop_entity = pe;
    }
    printf("%d; ", max_value[i]);
  }
  printf("\n");
  return 0;
}
#endif

int report_function()
{
  //if there are no relations, do nothing
  if (rel_list_head == NULL)
  {
    printf("none\n");
    return 0;
  }

  OrdRel* pointer = rel_list_head;

  return 0;
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
          {
            printf("%s [id = %d], ", entity->name, entity->entity_id);
          }
          entity = (Entity*) entity->table_next;
        }
        printf("\n");
    }
  }
}

void deb_print_sub_relations(Relation* rel)
{
  SubRelation* sub_rel = (SubRelation*) rel->first_sub_relation;
  SubRelation* prev_sub_rel = NULL;
  while (sub_rel != NULL)
  {
    if (sub_rel->valid == 1)
    {
      Entity* src = sub_rel->source;
      Entity* dest = sub_rel->destination;
      printf("%s --%s--> %s\n", src->name, rel->name, dest->name);
      prev_sub_rel = sub_rel;
      sub_rel = (SubRelation*) sub_rel->table_next;
    }
    else
    {
      sub_rel = delete_invalid_relation(sub_rel);
      if (prev_sub_rel == NULL)
        rel->first_sub_relation = sub_rel;
      else
        prev_sub_rel->table_next = sub_rel;
    }
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

  int opcode;
  while (1)
  {
    fgets(input_string, 160, stdin);
    opcode = generate_opcode(input_string);

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
        handle_relation_creation(argument0, argument1, argument2);
        #ifdef deb
          deb_print_relations();
        #endif
        break;
      }
      case 2: //delent
      {
        get_argument(input_string, argument0, 7);
        delent_function(argument0);
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
        delrel_function(argument0, argument1, argument2);
        #ifdef deb
          deb_print_relations();
        #endif
        break;
      }
      case 4: //report
      {
        report_function();
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
