#include "ctype.h"
#include "sys/types.h"
#include "stdio.h"
#include "stdlib.h"
#include "define.h"
#include "struct.h"


path_func  hear_whistle;
path_func  respond_summon;


Path_Data :: Path_Data( )
{
  step        = 0;
  length      = 0;
  directions  = NULL;  
  summoner    = NULL;
}


Path_Data :: ~Path_Data( )
{
  if( directions != NULL )
    delete [] directions;
}


/*
 *   DISTANCE FUNCTIONS 
 */


void exec_range( char_data* ch, int range, path_func* function,
  char* arg1 )
{
  room_array       list;
  room_array       next;
  char_data*        rch;
  exit_data*       exit;
  room_data*     center  = ch->in_room;
  room_data*       room;
  room_data*    to_room;

  if( range > 24 )
    range = 24;

  center->distance  = 0;
  list             += center;

  /* LOOP THROUGH ROOMS */

  for( int i = 1; i < range; i++ ) {
    for( int j = 0; j < list; j++ ) { 
      room = list[j];
      for( int k = 0; k < room->exits; k++ ) {
        exit    = room->exits[k]; 
        to_room = exit->to_room;
        if( to_room->distance == MAX_INTEGER ) {
          to_room->distance = i;
          next += to_room;
          for( int n = 0; n < to_room->contents; n++ )
            if( ( rch = character( to_room->contents[n] ) ) != NULL )
              ( *function )( rch, ch, arg1,
                dir_table[ exit->direction ].reverse, i+1 );
          }
        }
      }
    if( is_empty( next ) )
      break;
    swap( list.list, next.list );
    swap( list.size, next.size );
    clear( next );
    }

  /* CLEAN UP */

  clear( list );

  list             += center;
  center->distance  = MAX_INTEGER;
 
  for( ; ; ) {
    for( int i = 0; i < list; i++ ) {
      room = list[i];
      for( int j = 0; j < room->exits; j++ ) {
        to_room = room->exits[j]->to_room;
        if( to_room->distance != MAX_INTEGER ) {
          to_room->distance  = MAX_INTEGER;
          next              += to_room;
          }
        }
      }
    if( is_empty( next ) )
      break;
    swap( list.list, next.list );
    swap( list.size, next.size );
    clear( next );
    }
}


/*
 *   PATH ROUTINES
 */


path_data* make_path( room_data* start )
{
  room_array     list;
  room_array     next; 
  room_data*     room;
  room_data*  to_room;
  exit_data*     exit;
  path_data*     path;
  int          length;
  int             dir  [ 100 ];

  if( start->distance == 0 )
    return NULL;

  for( length = 0, room = start; room->distance != 0; ) {
    for( int i = 0; ; i++ ) {
      if( i == room->exits )
        return NULL;
      exit    = room->exits[i];
      to_room = exit->to_room;
      if( to_room->distance < room->distance ) {
        room = to_room;
        dir[length++] = exit->direction;
        break;
        }      
      }
    }     

  path              = new path_data;  
  path->step        = 0;
  path->length      = length;
  path->directions  = new int[ length ];

  memcpy( path->directions, dir, sizeof( int )*length );

  return path;
}


void add_path( char_data* ch, char_data* summoner )
{
  path_data*    path;
  event_data*  event;

  if( ( path = make_path( ch->in_room ) ) == NULL )
    return;

  stop_events( ch, execute_path );
  stop_events( ch, execute_wander );

  event          = new event_data( execute_path, ch );
  event->pointer = (void*) path;  
  path->summoner = summoner; 

  add_queue( event, 50 );
}


void execute_path( event_data* event )
{
  char_data*    ch  = (char_data*) event->owner;
  path_data*  path  = (path_data*) event->pointer;

  for( int i = 0; i < ch->in_room->contents; i++ )
    if( ch->in_room->contents[i] == path->summoner ) 
      goto end_path;

  move_char( ch, path->directions[ path->step++ ], FALSE );

  if( !ch->Is_Valid( ) )
    return;

  if( path->step < path->length ) {
    add_queue( event, 50 );
    return;
    }

  end_path:

  extract( event );

  if( !is_set( &ch->status, STAT_SENTINEL ) )    
    delay_wander( new event_data( execute_wander, ch ) );  
}


/* 
 *   WHISTLE ROUTINE
 */


const char* whistle_msg = 
  "Your incessant whistling seems to have attracted a disgruntled mail\
 daemon.\n\rHe stamps up to you and mutters a few strange words, then in\
 an\n\runcharacteristic gesture smiles happily and waves to you.  Finally\
 with\n\ra snap of his fingers he disappears in a cloud of dark smoke.\n\r";


obj_data* has_whistle( char_data* ch )
{
  obj_data* obj;

  for( int i = ch->contents.size-1 ; i > 0 ; i-- ) {
    if( ( obj = object( ch->contents[i] ) ) != NULL )
      if( obj->pIndexData->item_type == ITEM_WHISTLE )
        return obj;
    }

  for( int j = ch->wearing.size-1 ; j > 0 ; j-- ) {
    if( ( obj = object( ch->wearing[j] ) ) != NULL )
      if( obj->pIndexData->item_type == ITEM_WHISTLE )
        return obj;
    }

/* Changing this to be any worn or inventory item of type whistle
  if( ( obj = ch->Wearing( WEAR_HELD_R ) ) != NULL
    && obj->pIndexData->item_type == ITEM_WHISTLE ) 
    return obj;

  if( ( obj = ch->Wearing( WEAR_HELD_L ) ) != NULL
    && obj->pIndexData->item_type == ITEM_WHISTLE ) 
    return obj;
*/
  return NULL;
}


void do_whistle( char_data* ch, char* )
{
  obj_data*          obj;
  oprog_data*      oprog;
  player_data*        pc;
  int              range  = 15;

  if( !can_talk( ch, "whistle" ) )
    return;

  if( ( obj = has_whistle( ch ) ) != NULL ) {
    for( oprog = obj->pIndexData->oprog; oprog != NULL; oprog = oprog->next )
      if( oprog->trigger == OPROG_TRIGGER_USE ) {
        var_victim = NULL;
        var_mob    = NULL;
        var_room   = ch->in_room;
        var_ch     = ch;
        if( !execute( oprog ) )
          return;
        break;
        }
    if( oprog == NULL ) {
      send( ch, "You blow %s producing a loud shriek.\n\r", obj );
      send( *ch->array, "%s blows %s producing a loud shriek.\n\r", ch, obj );
      }
    range = obj->value[0];
    } 
  else {
    if( ch->shdata->race == RACE_LIZARD ) {
      send( ch,
        "You try to whistle but the best you can do is hiss loudly.\n\r" );
      send( *ch->array,
        "%s hisses loudly - you are not sure what it means.\n\r", ch );
      return;
      }
    send( ch, "You whistle as loud as you can.\n\r" );
    send( *ch->array, "%s whistles loudly.\n\r", ch );
    }

  exec_range( ch, range, hear_whistle );

  if( ( pc = player( ch ) ) != NULL && pc->whistle++ >= 3 ) {
    send( ch, whistle_msg );
    spell_silence( ch, ch, NULL, 10, 5 );
    }
}


void hear_whistle( char_data* victim, char_data* ch, char*,
  int dir, int )
{
  if( victim->position == POS_EXTRACTED 
    || !can_hear( victim ) ) 
    return;

  send( victim, "You hear a whistle from somewhere %s.\n\r",
    dir_table[dir].where );        

  if( victim->pcdata != NULL
    || !is_set( &victim->status, STAT_PET )
    || victim->leader != ch )
    return;

  if( victim->position == POS_RESTING ) 
    do_stand( victim, "" );

  if( is_set( &victim->status, STAT_FAMILIAR ) )
    send( ch, "You sense your familiar heard you.\n\r" );

  add_path( victim, ch );
}


/*
 *   SUMMON_HELP ROUTINE
 */


void summon_help( char_data* ch, char_data* )
{
  exec_range( ch, 15, respond_summon );
}


void respond_summon( char_data* victim, char_data* ch, char*,
  int, int )
{
  int       nation  = ch->species->nation;
  int        group  = ch->species->group;

  if( victim->pcdata != NULL || victim->position < POS_RESTING
    || !is_set( &victim->species->act_flags, ACT_ASSIST_GROUP ) )
    return;

  if( ( nation == NATION_NONE || nation != victim->species->nation )
    && ( group == GROUP_NONE || group != victim->species->group ) )
    return;

  set_bit( &victim->status, STAT_ALERT );

  if( is_set( &victim->species->act_flags, ACT_SUMMONABLE ) ) {
    if( victim->position == POS_RESTING ) 
      do_stand( victim, "" );
    add_path( victim, ch );
    }
}


/*
 *   MAP ROUTINES
 */


void do_map( char_data* ch, char* argument )
{
  int flags;
  int     x;
  int     y;

  if( !get_flags( ch, argument, &flags, "lv", "Map" ) )
    return;

  x = 60;
  y = 20;

  if( is_set( &flags, 0 ) ) { x = 100;  y = 40; } 
  if( is_set( &flags, 1 ) ) { x = 160;  y = 60; } 

  show_map( ch, y, x );
}


void plot( int map [][161], int length, int width, int pos, int index,
  int& top, int& bottom, char_data* ch )
{
  int l = pos%1024-512;
  int x = (pos/1024)%1024-512+width/2;
  int y = (pos/1024/1024)-512+length/2;

  if( l != 0
    || x < 0 || x >= width
    || y < 0 || y >= length ) 
    return;

  top    = min( top, y );
  bottom = max( bottom, y );

//  if( map[y][x] != ' ' ) {
//    if( map[y][x] != letter && map[y][x] != '*' )
//      map[y][x] = '?';
//    }
//  else
    map[y][x] = index;
}


int offset( int pos, int direction )
{
  int l = pos%1024;
  int x = (pos/1024)%1024;
  int y = (pos/1024/1024);

  switch( direction ) {
    case DIR_NORTH :  
      if( y-- == 0 )    
        return -1;  
      break;
    case DIR_SOUTH :  
      if( y++ == 1024 ) 
        return -1;  
      break;
    case DIR_WEST  :  
      if( x-- == 0 )    
        return -1;  
      break;
    case DIR_EAST  :  
      if( x++ == 1024 ) 
        return -1;  
      break;
    case DIR_UP    :  
      if( l-- == 0 )    
        return -1;  
      break;
    case DIR_DOWN  :  
      if( l++ == 1024 ) 
        return -1;  
      break;
    }
  return y*1024*1024+x*1024+l;
}


void show_map( char_data* ch, int length, int width )
{
  char* key[] = { 
    "Key:",
    "",
    "*  you",
    "O  room",
    "?  overlap",
    "+  door",
    "|  ns exit",
    "-  ew exit",
    ">  up exit",
    "<  down exit",
    "X  u&d exit" };
   
  int      sym_exit[] = { -2, -1, -2, -1 };

  int               map  [ 60 ][ 161 ];
  int             index;
  area_data*       area;
  exit_data*       exit;
  room_data*       room;
  room_data*    to_room;
  int               top  = length/2;
  int            bottom  = length/2;
  int               pos;
 
  /* ZERO MAP */

  for( int i = 0; i < 60; i++ ) { 
    for( int j = 0; j < 161; j++ )
      map[i][j] = 99;
    }

  map[length/2][width/2] = 90;

  for( area = area_list; area != NULL; area = area->next )
    for( room = area->room_first; room != NULL; room = room->next )
      room->distance = 0;

  /* GENERATE MAP */

  room_array*  list1  = new room_array;
  room_array*  list2  = new room_array;

  *list1 += ch->in_room;
  ch->in_room->distance = 1024*1024*512+1024*512+512;

  for( ; !is_empty( *list1 ); ) {
    for( int i = 0; i < *list1; i++ ) {
      room = list1->list[i];

      for( int j = 0; j < room->exits; j++ ) {
        exit    = room->exits[j];
        to_room = exit->to_room;
        index   = to_room->sector_type;
        pos     = offset( room->distance, exit->direction );

        plot( map, length, width, pos, is_set( &exit->exit_info, EX_ISDOOR ) ? 95 : sym_exit[ exit->direction ], top, bottom, ch );

        if( to_room->distance == 0 && ( pos = offset( pos, exit->direction ) ) != -1 ) {
          bool up   = exit_direction( to_room, DIR_UP );
          bool down = exit_direction( to_room, DIR_DOWN );  
          plot( map, length, width, pos, up ? ( down ? 98 : 97 ) : ( down ? 96 : index ), top, bottom, ch ); 
          to_room->distance = pos;
          *list2 += to_room;
	  }
        }
      }
    clear( *list1 );
    swap( list1, list2 );
    }

  delete list1;
  delete list2;

  /* DISPLAY MAP */


  for( int i = 0; i <= bottom-top; i++ ) {
     for( int j = 0; j <= width; j++ ) { 
       switch ( map[i+top][j] ) {
       	 case 99 :
       	   send( ch, " " );
       	   break;
         case 98 :
           send_color( ch, COLOR_WHITE, "X" );
           break;
         case 97 :
           send_color( ch, COLOR_WHITE, ">" );
           break;
         case 96 :
           send_color( ch, COLOR_WHITE, "<" );
           break;
         case 95 :
           send_color( ch, COLOR_WHITE, "+" );
           break;
         case -1 :
           send_color( ch, COLOR_WHITE, "-" );
           break;
         case -2 :
           send_color( ch, COLOR_WHITE, "|" );
           break;
         case 90 :
           send_color( ch, COLOR_BOLD_RED, "*" );
           break;
         case SECT_CITY :
           send_color( ch, COLOR_YELLOW, "=" );
           break;
         case SECT_FIELD :
           send_color( ch, COLOR_GREEN, "/" );
           break;
         case SECT_FOREST :
           send_color( ch, COLOR_GREEN, "#" );
           break;
         case SECT_HILLS :
           send_color( ch, COLOR_MAGENTA, ")" );
           break;
         case SECT_MOUNTAIN :
           send_color( ch, COLOR_BOLD_MAGENTA, "^" );
           break;
         case SECT_WATER_SURFACE :
           send_color( ch, COLOR_CYAN, "~" );
           break;
         case SECT_UNDERWATER :
           send_color( ch, COLOR_BOLD_BLUE, "w" );
           break;
         case SECT_RIVER :
           send_color( ch, COLOR_BLUE, "~" );
           break;
         case SECT_DESERT :
           send_color( ch, COLOR_BOLD_YELLOW, ":" );
           break;
         case SECT_CAVES :
           send_color( ch, COLOR_BLACK, "c" );
           break;
         case SECT_ROAD :
           send_color( ch, COLOR_STRONG, "I" );
           break;
         case SECT_SHALLOWS :
           send_color( ch, COLOR_CYAN, "=" );
           break;
         case SECT_ARCTIC :
           send_color( ch, COLOR_BOLD_WHITE, "a" );
           break;
         case SECT_SWAMP :
           send_color( ch, COLOR_BOLD_GREEN, "@" );
           break;
         case SECT_ICE :
           send_color( ch, COLOR_BOLD_CYAN, "i" );
           break;
         case SECT_STREAM :
           send_color( ch, COLOR_CYAN, "~" );
           break;
         case SECT_JUNGLE :
           send_color( ch, COLOR_BOLD_GREEN, "$" );
           break;
         case SECT_BEACH :
           send_color( ch, COLOR_YELLOW, "b" );
           break; 
         default :
           send_color( ch, COLOR_WHITE, "O" );
       }
     } 
   send( ch, "\n\r" );
  }   
  send( ch, "\n\rKey:\n\r" );
  send( ch, "----------------------------------------------------------------------\n\r\n\r" );
  send_color( ch, COLOR_BOLD_RED, "*" );
  send( ch, " - You       " );
  send_color( ch, COLOR_WHITE, "|" );
  send( ch, " - N/S Exit  " );
  send_color( ch, COLOR_WHITE, "-" );
  send( ch, " - E/W Exit  " );
  send_color( ch, COLOR_WHITE, ">" );
  send( ch, " - Up Exit   " );
  send_color( ch, COLOR_WHITE, "<" );
  send( ch, " - Down Exit " );
  send_color( ch, COLOR_WHITE, "+" );
  send( ch, " - Door      \n\r" );
  send_color( ch, COLOR_BOLD_GREEN, "$" );
  send( ch, " - Jungle    " );
  send_color( ch, COLOR_YELLOW, "b" );
  send( ch, " - Beach     " );
  send_color( ch, COLOR_CYAN, "~" );
  send( ch, " - Stream    " );
  send_color( ch, COLOR_BOLD_CYAN, "i" );
  send( ch, " - Ice       " );
  send_color( ch, COLOR_BOLD_GREEN, "@" );
  send( ch, " - Swamp     " );
  send_color( ch, COLOR_BOLD_WHITE, "a" );
  send( ch, " - Arctic    \n\r" );
  send_color( ch, COLOR_CYAN, "=" );
  send( ch, " - Shallows  " );
  send_color( ch, COLOR_STRONG, "I" );
  send( ch, " - Road      " );
  send_color( ch, COLOR_BLACK, "c" );
  send( ch, " - Caves     " );
  send_color( ch, COLOR_BOLD_YELLOW, ":" );
  send( ch, " - Desert    " );
  send_color( ch, COLOR_BLUE, "~" );
  send( ch, " - River     " );
  send_color( ch, COLOR_BOLD_BLUE, "w" );
  send( ch, " - UndWater  \n\r" );
  send_color( ch, COLOR_CYAN, "~" );
  send( ch, " - Stream    " );
  send_color( ch, COLOR_BOLD_MAGENTA, "^" );
  send( ch, " - Mountains " );
  send_color( ch, COLOR_MAGENTA, ")" );
  send( ch, " - Hills     " );
  send_color( ch, COLOR_GREEN, "#" );
  send( ch, " - Forest    " );
  send_color( ch, COLOR_GREEN, "/" );
  send( ch, " - Field     " );
  send_color( ch, COLOR_YELLOW, "=" );
  send( ch, " - City      \n\r" );
}
