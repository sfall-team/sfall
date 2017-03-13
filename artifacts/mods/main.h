
#include "..\scripting\headers\sfall.h"
#include "..\scripting\headers\define_extra.h"

variable ini := "ddraw.ini";
variable translationIni;

// Gets the integer value from ddraw.ini
procedure GetConfig(variable section, variable key, variable def) begin
   variable val := get_ini_setting(ini + "|" + section + "|" + key);
   if val == -1 then val := def;
   return val;      
end

// Gets the string value from ddraw.ini
procedure GetConfigStr(variable section, variable key, variable def) begin
   variable val := get_ini_string(ini + "|" + section + "|" + key);
   if val == -1 or val == "" then val := def;
   return val;
end

// Translates given string using Translations.ini
procedure translate(variable id, variable def) begin
   variable str := get_ini_string(translationIni + "|Sfall|" + id); 
   if not str or (strlen(str) == 0) then begin
      str := def;
   end      
   return str;
end

