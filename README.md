# fancydialog

Displays a GUI dialog box that users can interact with.
Designed to be integrated into shell scripts.

Usage:
./fancydialog [options] <element> [<element>...]
Options:
 --title <string>: Set the title of the main window
 --position <x,y>: Place the window here, default: center
 --size <w,h>: Set the window size
 --icon <icon name>: Set the icon for the window by name
 --iconfile <file>: Set the icon for the window from a file
 --separator <string>: In the output use this string to
		separate keys from values.  Defaults to '='
Elements:
 --add-label <string>: Adds a label, no return value
   --style 'Pango Style String'
     See: https://developer.gnome.org/pygtk/stable/pango-markup-language.html
     Example: --add-label "My Fancy Label" --style 'style="bold" size="x-large"'
 --add-separator: Adds a horizontal line
 --add-image <image_file>: Adds an image, the file must be a PNG
 --add-entry <label>: Adds a one line textbox, returns the contents
   --default <text>: The entry field is prefilled with this value
 --add-password <label>: An entry field where the text is obscured
 --add-calendar <label>: An entry where you can pick a date
   --format <formatstring>: Format of the output. See:
   https://developer.gnome.org/glib/stable/glib-GDateTime.html#g-date-time-format
 --add-checkbox <label>: A binary entry, returns True if selected, False if not
   --checked: The checkbox will be selected by default
 --add-switch <label>: Another binary entry, this one stylized like an on-off switch, default off
   --on: The switch will be in the 'on' position by default
 --add-combobox <label>: A dropdown menu that allows one selection
   --value <text>: This value will be added to the combox
                   Use this option multiple times to add additional entries
   --default <text>: This value will be added and automatically selected
 --add-file-selector <label>: Select a file on the filesystem
   --savefile: Switch to choosing a new file to save to
   --selectfolder: Switch to selecting an existing folder
   --createfolder: Switch to createing a new folder
 --add-slider <label>: Allows the user to select a value between 0 and 100
   --default <0-100>: Start at this value
   --min <value>: Select a different minimum value
   --max <value>: Select a different maximum value
 --add-font <label>: An entry field where you choose a system font
   --default <fontspec>: The default font selected
 --add-color <label>: An entry field where you choose a color
   --default <r,g,b>: The color selected by default
   --hexstr: Output a hex string #rrggbb instead of decimal
 --add-application <label>: An entry where you choose from one of the installed applications
   --default <application>: The default application

# Shell Script Integration
To use this with bash scripts the easiest way is to import the output into an
associative array so you can access each option individually.  The option name
is the label you specified for a field, and the value depends on which kind of
field you used.  

# Example

#!/bin/bash

declare -A opts
while IFS== read -r key value
do
	opts["$key"]="$value"
done < <(fancydialog --title "Example Window" \
			--add-entry "Enter some text" \
			--add-checkbox "Select this if you like" \
			--add-color "Favorite Color" \
				--hexstr \
			--add-combobox "Favorite Direction" \
				--value "North" --value "West" \
				--value "East" --value "South")

echo "You entered ${opts["Enter some text"]}"
if [ ${opts["Select this if you like"]} == "True" ]
then
	echo "You liked it!"
fi

echo "Your favorite color is ${opts["Favorite Color"]} and your favorite direction is ${opts["Favorite Direction"]}"
