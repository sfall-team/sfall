/**

  This library contains procedures which should ease working with sfall arrays.
  sfall v3.4 or higher is required

  WARNING!!!
  1. This library may contain bugs, so test each method before you use it.
  2. Author is not responsible for any damage to your mod or your savegames when using this library.
  It is provided AS IS.

  @author phobos2077

*/

#ifndef LIB_ARRAYS_H
#define LIB_ARRAYS_H

#include "sfall.h"

/*
  Generic array functions
*/
// returns True if the specified key exists in the associative array
procedure map_contains_key(variable arrayMap, variable key);

// push new item at the end of array, returns array
procedure array_push(variable array, variable item);

// remove last item from array and returns it's value
procedure array_pop(variable array);

// list of array keys (for lists it will return indexes 0, 1, 2, etc.)
procedure array_keys(variable array);

// list of array values (useful for maps)
procedure array_values(variable array);

// makes given array permanent and returns it
procedure array_fixed(variable array);

// returns temp array containing a subarray starting from $index with $count elements
// negative $index means index from the end of array
// negative $count means leave this many elements from the end of array
procedure array_slice(variable array, variable index, variable count);

// remove $count elements from array starting from $index, returns $array
// rules for $index and $count are the same as in array_slice()
procedure array_cut(variable array, variable index, variable count);

// substract arr2 values from arr1 and return arr1
procedure array_diff(variable arr1, variable arr2);

// Copy a slice of one array into another (will not resize)
procedure copy_array(variable src, variable srcPos, variable dest, variable dstPos, variable size);

// create exact copy of the array as a new temp array
procedure clone_array(variable array);

// true if arrays are equal, false otherwise
procedure arrays_equal(variable arr1, variable arr2);

// returns maximum element in array
procedure array_max(variable arr);

// returns minimum element in array
procedure array_min(variable arr);

// returns sum of array elements (or concatenated string, if elements are strings)
procedure array_sum(variable arr);

// returns random value from array
procedure array_random_value(variable arr);


procedure array_fill(variable arr, variable pos, variable count, variable value);

/**
 * Merge arr2 on top of the arr1
 */
procedure array_append(variable arr1, variable arr2);

procedure get_empty_array_index(variable array);

/*
 Functions for working with sets (add item to set, remove from set)
 Sets are simple arrays where any value could exist only once
*/
procedure add_array_set(variable array, variable item);
procedure remove_array_set(variable array, variable item);

// Creates a new array filled from a given array by transforming each value using given procedure name.
procedure array_transform(variable arr, variable valueFunc);

// Create a new temp array filled from a given array by transforming each key and value using given procedure name.
procedure array_transform_kv(variable arr, variable keyFunc, variable valueFunc);



/**
   Array of blocks functions (allows to use arrays as arrays of "objects", where "object" properties are bound to
   relative offsets in one array)
   Recommended use case:
   1) create a block for each "object"
   2) define macros like (example):
   #define TRAP_OFSET_TILE                      (5)
   #define trap_tile(array, index)              (array[index + TRAP_OFSET_TILE])
   #define trap_tile_set(array, index, value)   (array[index + TRAP_OFSET_TILE] := value)
   3) use them to read and write object properties;
   4) use while loop with step equal to blocksize to iterate over all objects
   5) 0 offset is always equal to object "ID" (starting index of block)

   DEPRECATED, use collections instead
 */

// Adds new empty place for a new block into array. Returns index of new block that was "created".
procedure add_array_block(variable arr, variable blocksize);

// Removes a block from an array by index.
procedure remove_array_block(variable arr, variable blocksize, variable index);

/**
 * Converts any array to string for debugging purposes
 */
#define debug_array_str(arr)     debug_array_str_deep(arr, 1, false)

// Prints contents of a given array to main message window, for debugging purposes.
#define display_array(arr)       display_msg(debug_array_str(arr))


/**
 * Saving/loading helpers
 */
// load array and create it (save) if it doesn't exist
procedure load_create_array(variable name, variable size);
// load array, delete it if exists, create new array and save it into $name slot
procedure get_saved_array_new(variable name, variable size);
// name - saved name, arr - two-dimensional array (array of arrays)
// arrays on both levels can be both lists or maps
procedure save_collection(variable name, variable arr);
// load collection previously saved with save_collection
procedure load_collection(variable name);
// get array pointed to by given sfall global; create it if doesn't exist
// the difference between this and load_create_array is that array itself might not be saved (but sfall global is always saved)
//procedure sfall_global_array(variable global, variable size);

/**
 * Loads a "saved" array. If it doesn't exist, creates it (with a given size).
 */
#define load_create_array_map(name)    (load_create_array(name, -1))
#define get_saved_array_new_map(name)    (get_saved_array_new(name, -1))
//#define sfall_global_array_map(name)    (sfall_global_array(name, -1))


// IMPLEMENTATION

procedure map_contains_key(variable arrayMap, variable key) begin
   variable i;
   for (i := 0; i < len_array(arrayMap); i++) begin
      if (array_key(arrayMap, i) == key) then return true;
   end
   return false;
end

/**
 * Returns first index of zero value in a list array.
 * @arg {list} array
 * @ret {int}
 */
procedure get_empty_array_index(variable array) begin
   variable zero := false;
   variable i := 0;
   while i < len_array(array) and not(zero) do begin
      if (array[i] == 0) then begin
         zero := true; // break
      end else begin
         i++;
      end
   end
   return i;
end

/**
 * Pushes new item to the end of a list array and returns the array.
 * @arg {list} array
 * @arg {any} item
 * @ret {list}
 */
procedure array_push(variable array, variable item) begin
   variable n;
   n := len_array(array);
   resize_array(array, n + 1);
   set_array(array, n, item);
   return array;
end

/**
 * Removes last item from list array (reducing it's size by 1) and returns it's value.
 * @arg {list} array
 * @ret {mixed}
 */
procedure array_pop(variable array) begin
   variable n, ret;
   n := len_array(array) - 1;
   if (n >= 0) then begin
      ret := get_array(array, n);
      resize_array(array, n);
      return ret;
   end
   return 0;
end

/** 
 * Returns a temp list of keys from a given map array.
 * @arg {array}
 * @ret {list}
 */
procedure array_keys(variable array) begin
   variable tmp, i, len;
   len := len_array(array);
   tmp := temp_array_list(len);
   i := 0;
   while (i < len) do begin
      set_array(tmp, i, array_key(array, i));
      i++;
   end
   return tmp;
end

/**
 * Returns a temp list of values from a given map array.
 * @arg {array} array
 * @ret {array}
 */
procedure array_values(variable array) begin
   variable v, tmp, i, len;
   len := len_array(array);
   tmp := temp_array_list(len);
   i := 0;
   while (i < len) do begin
      set_array(tmp, i, get_array(array, array_key(array, i)));
      i++;
   end
   return tmp;
end

/**
 * Sets given array as permanent and returns it.
 * @arg {array} array
 * @ret {array}
 */
procedure array_fixed(variable array) begin
   fix_array(array);
   return array;
end

/**
 * Returns a slice of a given list array as a new temp array.
 * @arg {list} array
 * @arg {int} index - Start position to slice from.
 * @arg {int} count - Number of elements to slice.
 * @ret {list}
 */
procedure array_slice(variable array, variable index, variable count) begin
   variable tmp, i, n;
   n := len_array(array);
   if (n <= 0) then return temp_array_list(0);
   if (index < 0) then
      index := n + index;
   if (count < 0) then
      count := n - index + count;
   else if (index + count > n) then
      count := n - index;
   tmp := temp_array_list(count);
   for (i := 0; i < count; i++) begin
      tmp[i] := array[index + i];
   end
   return tmp;
end

/**
 * Removes a slice of given list array and returns it.
 * @arg {list} array
 * @arg {int} index - Start position to remove from.
 * @arg {int} count - Number of elements to remove.
 * @ret {list}
 */
procedure array_cut(variable array, variable index, variable count) begin
   variable i, n;
   n := len_array(array);
   if (n <= 0) then return;
   if (index < 0) then
      index := n + index;
   if (count < 0) then
      count := n - index + count;
   else if (index + count > n) then
      count := n - index;
   for (i := index; i < (n - count); i++) begin
      array[i] := array[i + count];
   end
   resize_array(array, n - count);
   return array;
end

/**
 * Removes all values in arr1 that also present in arr2 and returns arr1.
 * @arg {array} arr1
 * @arg {array} arr2
 * @ret {array}
 */
procedure array_diff(variable arr1, variable arr2) begin
   variable i, v, isMap;
   isMap := array_is_map(arr1);
   foreach (v in arr2) begin
      i := scan_array(arr1, v);
      if (i != -1) then begin
         if (isMap) then
            unset_array(arr1, i);
         else
            call array_cut(arr1, i, 1);
      end
   end
   return arr1;
end

/**
 * Copy a slice of one array into another (will not resize)
 * @arg {list} src - Source array.
 * @arg {int} srcPos - Position in source array.
 * @arg {list} dest - Destination array.
 * @arg {int} dstPos - Position in destination array.
 * @arg {int} size - Number of elements to copy.
 */
procedure copy_array(variable src, variable srcPos, variable dest, variable dstPos, variable size) begin
  variable i := 0;
  if (srcPos + size > len_array(src)) then size := len_array(src) - srcPos; // this should prevent access to offset that don't exist
  while (i < size) do begin
    dest[dstPos + i] := src[srcPos + i];
    i++;
  end
end

/**
 * Creates a shallow copy of the array as a new temp array.
 * @arg {array} array
 * @ret {array}
 */
procedure clone_array(variable array) begin
   variable new, k, v;
   if (array_is_map(array)) then
      new := temp_array_map;
   else
      new := temp_array_list(len_array(array));
   foreach k: v in array begin
      new[k] := v;
   end
   return new;
end

/**
 * Compares two arrays (list or map) and returns true if they have identical values in the same order.
 * @arg {array} arr1
 * @arg {array} arr2
 * @ret {bool}
 */
procedure arrays_equal(variable arr1, variable arr2) begin
   variable n, i, k1, k2;
   if (array_is_map(arr1) != array_is_map(arr2)) then
      return false;
   n := len_array(arr1);
   if (n != len_array(arr2)) then
      return false;
   i := 0;
   while (i < n) do begin
      k1 := array_key(arr1, i);
      k2 := array_key(arr2, i);
      if (k1 != k2) then
         return false;
      if (get_array(arr1, k1) != get_array(arr2, k2)) then
         return false;
      i++;
   end
   return true;
end

/**
 * Returns maximum element in array.
 * @arg {array} arr
 * @ret {mixed}
 */
procedure array_max(variable arr) begin
   variable v, max;
   max := 0;
   foreach v in arr begin
      if (max == 0 or v > max) then
         max := v;
   end
   return max;
end

/**
 * Returns minimum element in array.
 * @arg {array} arr
 * @ret {mixed}
 */
procedure array_min(variable arr) begin
   variable v, min;
   min := 0;
   foreach v in arr begin
      if (min == 0 or v < min) then
         min := v;
   end
   return min;
end

/**
 * Returns sum of array elements (or concatenated string, if elements are strings).
 * @arg {array} arr
 * @ret {mixed}
 */
procedure array_sum(variable arr) begin
   variable v, sum;
   sum := 0;
   foreach v in arr begin
      sum += v;
   end
   return sum;
end

/**
 * Returns a random value from a given list array.
 * @arg {list} arr
 * @ret {any}
 */
procedure array_random_value(variable arr) begin
   return get_array(arr, array_key(arr, random(0, len_array(arr) - 1)));
end


#define ARRAY_SET_BLOCK_SIZE  (10)

/**
 * Array set is a list array that is used as a set of unique values (where no diplicate value is allowed).
 * Tries to add new value to a set and returns true if it was just added.
 * @arg {list} array - List array to use as a set.
 * @arg {any} item
 * @ret {bool}
 */
procedure add_array_set(variable array, variable item) begin
   variable i := 0;
   variable len;
   variable exist := false;
   variable zero := false;
   len := len_array(array);

   // search for first empty space and also check if item exists
   while i < len and not(zero) do begin
      if (array[i] == 0) then begin
         zero := true; // break
         i--;
      end else if (array[i] == item) then exist := true;
      i++;
   end
   if not(exist) then begin
      // if no empty space, resize array
      if (i == len) then begin
         resize_array(array, len + ARRAY_SET_BLOCK_SIZE);
      end
      set_array(array, i, item);
      return true;
   end
   return false;
end

/**
 * Remove value from a set (list array). Returns true if item was actually found and removed.
 * @arg {list} array - List array to use as a set.
 * @arg {any} item
 * @ret {bool}
 */
procedure remove_array_set(variable array, variable item) begin
   variable i := 0;
   variable len;
   variable found_at := -1;
   variable zero := false;

   len := len_array(array);
   // search for first empty space and also check if item exists
   while (i < len and not(zero)) do begin
      if (array[i] == 0) then begin
         zero := true; // break
         i--;
      end else if (array[i] == item) then found_at := i;
      i++;
   end
   if (found_at != -1) then begin
      array[found_at] := array[i - 1];
      array[i - 1] := 0;
      return true;
   end
   return false;
end

#undef ARRAY_SET_BLOCK_SIZE

/**
 * Returns a new array containing only those items from *arr* for which *filterFunc* returns true.
 * @arg {array} arr - Array to use values from. Can be map or list.
 * @arg {string} filterFunc - A name of procedure that accepts value from arr and returns true if it should be copied to the new array.
 * @arg {bool} negate - If true, reverses the result of filterFunc so that only "false" values will be copied to the new array.
 * @ret {array}
 */
procedure array_filter(variable arr, variable filterFunc, variable negate := false) begin
   variable k, v,
      isMap := array_is_map(arr),
      retArr := temp_array_map if isMap else temp_array_list(0);
   foreach (k: v in arr) begin
      if ((filterFunc(v) != 0) == (not negate)) then begin
         if (isMap) then
            set_array(retArr, k, v);
         else
            call array_push(retArr, v);
      end
   end
   return retArr;
end

/**
 * Creates a new array filled from a given array by transforming each value using given procedure name.
 * @arg {array} arr - Array to use values from.
 * @arg {string} valueFunc - A name of procedure that accepts value from arr and returns a new value.
 * @ret {array}
 */
procedure array_transform(variable arr, variable valueFunc) begin
   variable k, v, retArr := temp_array_map if array_is_map(arr) else temp_array(len_array(arr), 0);
   foreach (k: v in arr) begin
      retArr[k] := valueFunc(v);
   end
   return retArr;
end

/**
 * Creates a new temp array filled from a given array by transforming each key and value using given procedure name.
 * @arg {array} arr - Array to use keys and values from.
 * @arg {string} keyFunc - A name of procedure that accepts key from arr and returns a new key for the new array.
 * @arg {string} valueFunc - A name of procedure that accepts value from arr and returns a new value.
 * @ret {map}
 */
procedure array_transform_kv(variable arr, variable keyFunc, variable valueFunc) begin
   variable k, v, retArr := temp_array_map;
   foreach (k: v in arr) begin
      retArr[keyFunc(k)] := valueFunc(v);
   end
   return retArr;
end

/**
 * Converts given array into a new map where keys are array values and all values are set to specified value.
 * @arg {array} arr
 * @ret {array}
 */
procedure array_to_set(variable arr, variable value := 1) begin
   variable v, retArr := temp_array_map;
   foreach (v in arr) begin
      retArr[v] := value;
   end
   return retArr;
end


#define ARRAY_EMPTY_INDEX   (-1)

/**
 * Adds new empty place for a new block into array. Returns index of new block that was "created".
 * @arg {list} arr - array to add "block" into
 * @arg {int} blocksize - block size
 * @ret {int} - index of added block
 * @deprecated - use collections instead
 */
procedure add_array_block(variable arr, variable blocksize) begin
   variable begin
      index := 0;
      zero := false;
      tile;
      elev;
   end
   // find empty array index
   index := 0;
   while index < len_array(arr) and not(zero) do begin
      if (get_array(arr, index) == ARRAY_EMPTY_INDEX) then begin
         // this index is empty, place struct here
         zero := true; // break
      end else begin
         index += blocksize;
      end
   end
   if (index == len_array(arr)) then begin
      resize_array(arr, index + blocksize);
   end
   set_array(arr, index, index);
   return index;
end

/**
 * Removes a block from an array by index
 * @arg {list} arr - array to remove "block" from
 * @arg {int} blocksize - block size
 * @arg {int} index - index to remove
 * @deprecated - use collections instead
 */
procedure remove_array_block(variable arr, variable blocksize, variable index) begin
   variable len;
   len := len_array(arr);
   if (index + blocksize == len) then begin
      // if this is last block, reduce the array
      resize_array(arr, len - blocksize);
   end else begin
      // mark block as empty
      set_array(arr, index, ARRAY_EMPTY_INDEX);
      // null other part of block - just in case...
      call array_fill(arr, index + 1, blocksize - 1, 0);
   end
end

#undef ARRAY_EMPTY_INDEX

/**
 * Fill array (or it's part) with the same value.
 * @arg {list} arr
 * @arg {int} pos - starting position
 * @arg {int} count - number of items to fill (use -1 to fill to the end of the array);
 * @arg {any} value - value to set;
 */
procedure array_fill(variable arr, variable pos, variable count, variable value) begin
   variable i := 0;
   if (count == -1 or (pos + count > len_array(arr))) then count := len_array(arr) - pos; // this should prevent write to illegal offsets
   while (i < count) do begin
      arr[pos + i] := value;
      i++;
   end
   return arr;
end

/**
 * Adds all the values of the second array to the first array. If arr1 is a map then for values with same keys, values from arr2 will take priority.
 * @arg {array} arr1
 * @arg {array} arr2
 * @ret {array} - the first array after modification
 */
procedure array_append(variable arr1, variable arr2) begin
   variable arr1_len;
   if (array_is_map(arr1)) then begin
      variable k, v;
      foreach (k: v in arr2) begin
         arr1[k] := v;
      end
   end else begin
      arr1_len := len_array(arr1);
      resize_array(arr1, arr1_len + len_array(arr2));
      call copy_array(arr2, 0, arr1, arr1_len, len_array(arr2));
   end
   return arr1;
end

/**
 * Creates a new temp array with all values from arr1 and arr2. If arr1 is a map then for values with same keys, values from arr2 will take priority.
 * @arg {array} arr1
 * @arg {array} arr2
 * @ret {array} - the created temp array
 */
#define array_concat(arr1, arr2)       array_append(clone_array(arr1), arr2)

/**
 * Loads a "saved" array. If it doesn't exist, creates it (with a given size).
 * @arg {string} name - saved array name/key
 * @arg {int} size - size of array to create
 * @ret {array} - the created/loaded array
 */
procedure load_create_array(variable name, variable size) begin
   variable arr;
   arr := load_array(name);
   if (arr == 0) then begin
      arr := create_array(size, 0);
      save_array(name, arr);
   end
   return arr;
end

/**
 * Creates and returns a new "saved" array. If array already existed with this name, frees it.
 * @arg {string} name - saved array name/key
 * @arg {int} size - size of array to create
 * @ret {array} - the new array
 */
procedure get_saved_array_new(variable name, variable size) begin
   variable arr;
   arr := load_array(name);
   if (arr) then
      free_array(arr);
   arr := create_array(size, 0);
   save_array(name, arr);
   return arr;
end

#define _ITEM_NAME(colname, itemkey)      ""+colname+"."+itemkey

/**
 * A collection is a 2-level-deep saved array (a saved array containing other saved arrays as values).
 * @arg {string} name - name/key of a "root" array
 * @arg {array} arr - collection array
 */
procedure save_collection(variable name, variable arr) begin
   variable k, v, keys, oldKeys;
   keys := array_keys(arr);
   oldKeys := load_array(name);
   if (oldKeys) then begin // delete removed items
      call array_diff(oldKeys, keys);
      foreach (k in oldKeys)
         free_array(load_array(_ITEM_NAME(name, k)));
   end
   save_array(name, keys);
   foreach (k: v in arr) begin
      save_array(_ITEM_NAME(name, k), v);
   end
end

/**
 * Loads collection by name.
 * @arg {string} name - name/key of a "root" array
 * @ret {array}
 */
procedure load_collection(variable name) begin
   variable k, v, keys, arr;
   keys := load_array(name);
   if (keys) then begin
      arr := create_array_map; // resulting collection
      foreach k in keys begin
         set_array(arr, k, load_array(_ITEM_NAME(name, k)));
      end
      return arr;
   end
   return 0;
end
#undef _ITEM_NAME


/**
  Different utility functions...
*/

/**
 * Formats array contents into a string with a given level of recursion. For debugging.
 * @arg {array} arr
 * @arg {int} levels - recursion level
 * @arg {bool} prefix - true to include a prefix with item count
 * @ret {string}
 */
procedure debug_array_str_deep(variable arr, variable levels, variable prefix := false) begin
#define _newline if (levels > 1) then s += "\n";
#define _indent ii := 0; while (ii < levels - 1) do begin s += "   "; ii++; end
#define _value(v) (v if (levels <= 1 or not array_exists(v)) else debug_array_str_deep(v, levels - 1))
   variable i := 0, ii, k, v, s, len;
   len := len_array(arr);
   if (array_is_map(arr)) then begin  // print assoc array
      s := ("Map("+len+"): {") if prefix else "{";
      while i < len do begin
         _newline
         k := array_key(arr, i);
         v := get_array(arr, k);
         _indent
         s += k + ": " + _value(v);
         if i < (len - 1) then s += ", ";
         i++;
      end
      //if (strlen(s) > 254) then s := substr(s, 0, 251) + "...";
      _newline
      s += "}";
   end else begin  // print list
      s := ("List("+len+"): [") if prefix else "[";
      _newline
      while i < len do begin
         _newline
         v := get_array(arr, i);
         _indent
         s += _value(v);
         if i < (len - 1) then s += ", ";
         i++;
      end
      //if (strlen(s) > 254) then s := substr(s, 0, 251) + "...";
      _newline
      s += "]";
   end
   return s;
#undef _newline
#undef _indent
#undef _value
end

// NUKES all saved arrays. Don't use in production code.
procedure _PURGE_all_saved_arrays begin
   variable saved, key;
   saved := list_saved_arrays;
   foreach (key in saved)
      free_array(load_array(key));
end

#endif

