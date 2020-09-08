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

#include <windows.h>

#define GLIDEDLL "glide3x.dll"
#define GLIDEEXE "glide-init.exe"

struct version
{
    uint16_t v1;
    uint16_t v2;
    uint16_t v3;
    uint16_t v4;
};

static size_t Offset_dll_width = 0x0;
static size_t Offset_dll_height = 0x0;
static size_t Offset_exe = 0x0;
static size_t Size_exe = 0;

static bool GetVersionForFile(const char *filename, struct version *output) // WinAPI syntax and naming schemes.
{
    bool retval = false;
    DWORD versionhandle = 0;
    DWORD versionsize = GetFileVersionInfoSize(filename, &versionhandle);
    if(versionsize == 0)
    {
        printf("Err: GetOffsetsForGlide failed at GetFileVersionInfoSize. %lu\n", GetLastError());
        printf("   : https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes\n"); // TODO: API lookup error code handling.
        goto GetVersionForFile_dead;
    }
    LPSTR versionbuffer = malloc(versionsize);
    if(versionbuffer == NULL)
    {
        printf("Err: GetOffsetsForGlide failed at malloc for versionbuffer with %lu bytes.\n", versionsize);
        goto GetVersionForFile_dead;
    }
    if(GetFileVersionInfo(filename, versionhandle, versionsize, versionbuffer) == FALSE)
    {
        printf("Err: GetOffsetsForGlide failed at GetFileVersionInfo.\n");
        goto GetVersionForFile_buffer;
    }
    LPBYTE buffer = NULL;
    UINT buffersize = 0;
    if(VerQueryValue(versionbuffer, "\\", (LPVOID *)&buffer, &buffersize) == FALSE)
    {
        printf("Err: GetOffsetsForGlide failed at VerQueryValue.\n");
        goto GetVersionForFile_buffer;
    }
    if(buffersize == 0)
    {
        printf("Err: GetOffsetsForGlide failed at VerQueryValue, version data size is zero.\n");
        goto GetVersionForFile_buffer;
    }
    VS_FIXEDFILEINFO *versioninfo = (VS_FIXEDFILEINFO *)buffer;
    if(versioninfo->dwSignature != 0xFEEF04BD)
    {
        printf("Err: GetOffsetsForGlide failed at VerQueryValue, version info signature is not valid. %08X\n", versioninfo->dwSignature);
        goto GetVersionForFile_buffer;
    }
    output->v1 = HIWORD(versioninfo->dwFileVersionMS);
    output->v2 = LOWORD(versioninfo->dwFileVersionMS);
    output->v3 = HIWORD(versioninfo->dwFileVersionLS);
    output->v4 = LOWORD(versioninfo->dwFileVersionLS);
    retval = true;
GetVersionForFile_buffer:
    free(versionbuffer);
    versionbuffer = NULL;
GetVersionForFile_dead:
    return retval;
}
static bool GetOffsets()
{
    bool gooddll = false;
    bool goodexe = false;
    struct version v;
    if(GetVersionForFile(GLIDEDLL, &v) == false)
    {
        goto GetOffsets_dead;
    }
    if(v.v1 == 1 && v.v2 == 4 && v.v3 == 4 && v.v4 == 21) // Sven's 1.4e
    {
        printf("DLL %d.%d.%d.%d == Sven's 1.4e detected.\n", v.v1, v.v2, v.v3, v.v4);
        Offset_dll_width = 0x7DF8;
        Offset_dll_height = 0x7E04;
        gooddll = true;
    }
    else if(v.v1 == 1 && v.v2 == 4 && v.v3 == 6 && v.v4 == 1) // D2SE's edited
    {
        printf("DLL %d.%d.%d.%d == D2SE's edited 1.4e detected.\n", v.v1, v.v2, v.v3, v.v4);
        Offset_dll_width = 0x8048;
        Offset_dll_height = 0x8054;
        gooddll = true;
    }
    if(gooddll == false)
    {
        printf("Err: GetOffsets failed for DLL, unknown version.\n");
        printf("   : %d.%d.%d.%d\n", v.v1, v.v2, v.v3, v.v4);
    }
    if(GetVersionForFile(GLIDEEXE, &v) == false)
    {
        goto GetOffsets_dead;
    }
    if(v.v1 == 2 && v.v2 == 0 && v.v3 == 1 && v.v4 == 24) // Sven's 1.4e
    {
        printf("EXE %d.%d.%d.%d == Sven's 1.4e detected.\n", v.v1, v.v2, v.v3, v.v4);
        Offset_exe = 0x1890A;
        Size_exe = 10;
        goodexe = true;
    }
    if(goodexe == false)
    {
        printf("Err: GetOffsets failed for EXE, unknown version.\n");
        printf("   : %d.%d.%d.%d\n", v.v1, v.v2, v.v3, v.v4);
    }
    printf("\n");
GetOffsets_dead:
    return gooddll && goodexe;
}

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
    if(GetOffsets() == false)
    {
        goto main_dead;
    }
    char buffer[12];
    FILE *glide_dll = fopen(GLIDEDLL, "r+");
    if(glide_dll == NULL)
    {
        PrintMissingFile(GLIDEDLL);
        goto main_dead;
    }
    FILE *glide_exe = fopen(GLIDEEXE, "r+");
    if(glide_exe == NULL)
    {
        PrintMissingFile(GLIDEEXE);
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
