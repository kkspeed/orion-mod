/* gok-predictor.c
*
* Copyright 2002 Sun Microsystems, Inc.,
* Copyright 2002 University Of Toronto
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*/

#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



/*
  constants
*/
#define ARRAYLIST_INITIAL_CAPACITY 256
#define ARRAYLIST_CAPACITY_DELTA 256

#define Object char

static const size_t object_size = sizeof(Object);
/*
  structures
*/
struct Arraylist_Struct {
  int _current_capacity;
  Object *_data;
  int _size;
};


void arraylist_free(const Arraylist list)
{
  free(list->_data);
  free(list);
}

Arraylist arraylist_create()
{
  Arraylist list;
  list = malloc(sizeof(struct Arraylist_Struct));
  list->_current_capacity = ARRAYLIST_INITIAL_CAPACITY;
  list->_data = malloc(object_size * list->_current_capacity);
  list->_size = 0;
  return list;
}

int arraylist_add(const Arraylist list, Object object)
{
  int old_size = arraylist_size(list);
  int new_capacity;
  Object *new_data;

  (list->_size)++;
  if (old_size == list->_current_capacity)
    {
      new_capacity = list->_current_capacity + ARRAYLIST_CAPACITY_DELTA;
      new_data = malloc(object_size * new_capacity);
      memcpy(new_data, list->_data, object_size * old_size);
      free(list->_data);
      (list->_data) = new_data;
      list->_current_capacity = new_capacity;
    }
  (list->_data)[old_size] = object;
  return 1;
}

int arraylist_is_empty(const Arraylist list)
{
  return (0 == arraylist_size(list));
}

int arraylist_size(const Arraylist list)
{
  return list->_size;
}

Object arraylist_get(const Arraylist list, const int index)
{
  return list->_data[index];
}

Object * arraylist_getData(const Arraylist list)
{
  return list->_data;
}

void arraylist_clear(const Arraylist list)
{
  list->_data = realloc(list->_data, object_size * ARRAYLIST_INITIAL_CAPACITY);
  list->_current_capacity = ARRAYLIST_INITIAL_CAPACITY;
  list->_size = 0;
}
