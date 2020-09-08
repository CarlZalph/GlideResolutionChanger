/*
    GlideResolutionChanger sets the glide wrapper's hardcoded values to a custom one.
    Copyright (C) 2020  CarlZalph

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static const size_t Offset_dll_width = 0x7DF8;
static const size_t Offset_dll_height = 0x7E04;
static const size_t Offset_exe = 0x1890A;
static const size_t Size_exe = 10;

static bool GetResolutionFromUser(char *buffer, size_t buffersize, uint32_t *width, uint32_t *height)
{
    printf("Window Dimensions?  Ex: 1600 1200\n");
    size_t bufferpos = 0;
    fflush(stdin);
    if(fgets(buffer, buffersize, stdin) != buffer)
    {
        printf("Err: What you entered was not valid in the slightest, and you broke fgets while doing so.\n");
        printf("   : I hope you're happy.  This is not supposed to happen.  You did this on purpose.\n");
        return false;
    }
    for(size_t i=0;i<buffersize;++i)
    {
        char c = buffer[i];
        if((c >= '0' && c <= '9') || c == ' ')
        {
            buffer[bufferpos] = c;
            ++bufferpos;
        }
    }
    buffer[bufferpos] = '\0';
    int scans = sscanf(buffer, "%u %u", width, height);
    if(scans != 2)
    {
        printf("Err: Your input was not acceptable.\n");
        printf("   : Valid input: # #\n");
        printf("   : Your input: %s\n", buffer);
        return false;
    }
    return true;
}

static void PrintMissingFile(const char *filename)
{
    printf("Err: %s is missing from the current working directory.\n", filename);
    printf("   : Please run this program in the same directory as %s.\n", filename);
    printf("   : If running via a terminal, then set the current directory to the above folder.\n");
}

int main(int argc, char **argv)
{
    char buffer[12];
    FILE *glide_dll = fopen("glide3x.dll", "r+");
    if(glide_dll == NULL)
    {
        PrintMissingFile("glide3x.dll");
        goto main_dead;
    }
    FILE *glide_exe = fopen("glide-init.exe", "r+");
    if(glide_exe == NULL)
    {
        PrintMissingFile("glide-init.exe");
        goto main_dll;
    }
    uint32_t width, height;
    if(GetResolutionFromUser(buffer, sizeof(buffer), &width, &height) == false)
    {
        goto main_exe;
    }
    buffer[0] = '\0';
    int bufferpos = snprintf(buffer, Size_exe, "%ux%u", width, height);
    if(bufferpos < 0 || bufferpos >= Size_exe)
    {
        snprintf(buffer, Size_exe, "<!BIG!>");
    }
    printf("Setting window resolution to: %ux%u\n", width, height);
    printf("Which will be displayed in glide-init.exe as: %s\n", buffer);
    fseek(glide_dll, Offset_dll_width, SEEK_SET);
    fwrite(&width, sizeof(width), 1, glide_dll);
    fseek(glide_dll, Offset_dll_height, SEEK_SET);
    fwrite(&height, sizeof(height), 1, glide_dll);
    fseek(glide_exe, Offset_exe, SEEK_SET);
    fwrite(buffer, bufferpos+1, 1, glide_exe);
main_exe:
    fclose(glide_exe);
    glide_exe = NULL;
main_dll:
    fclose(glide_dll);
    glide_dll = NULL;
main_dead:
    printf("\nPress return/enter to quit.\n");
    fflush(stdin);
    fgets(buffer, sizeof(buffer), stdin);
    return 0;
}
