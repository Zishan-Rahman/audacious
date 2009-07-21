/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */
#include "vfs.h"
#include <string.h>
#include <stdlib.h>
#include <glib/gprintf.h>
/**
 * @file vfs_common.c
 * Contains code for various common VFS-stream related operations,
 * which do not need specific VFS backend implementations. Provides
 * routines for string reading and writing and functions for
 * reading and writing endianess-dependant integer values.
 */


/**
 * Writes a character to a stream.
 *
 * @param c A character to write to the stream.
 * @param stream A #VFSFile object representing the stream.
 * @return The character on success, or EOF.
 */
gint vfs_fputc(gint c, VFSFile *stream)
{
    guchar uc = (guchar) c;

    if (!vfs_fwrite(&uc, 1, 1, stream)) {
        return EOF;
    }

    return uc;
}

/**
 * Reads a string of characters from a stream, ending in newline or EOF.
 *
 * @param s A buffer to put the string in.
 * @param n The amount of characters to read.
 * @param stream A #VFSFile object representing the stream.
 * @return The string on success, or NULL.
 */
gchar *vfs_fgets(gchar *s, gint n, VFSFile *stream)
{
    gint c;
    register gchar *p;

    if(n<=0) return NULL;

    p = s;

    while (--n) {
        if ((c = vfs_getc(stream))== EOF) {
            break;
        }
        if ((*p++ = c) == '\n') {
            break;
        }
    }
    if (p > s) {
        *p = 0;
        return s;
    }

    return NULL;
}

/**
 * Writes a string to a VFS stream.
 *
 * @param s A string to write to the stream.
 * @param stream A #VFSFile object representing the stream.
 * @eturn The amount of bytes written.
 */
gint vfs_fputs(const gchar *s, VFSFile *stream)
{
	size_t n = strlen(s);

	return ((vfs_fwrite(s, 1, n, stream) == n) ? n : EOF);
}

/**
 * Writes a formatted string to a VFS stream via a va_list of args.
 *
 * @param stream A #VFSFile object representing the stream.
 * @param format A printf-style format string.
 * @param args A va_list of args to use.
 * @return value The amount of bytes written.
 */
gint vfs_vfprintf(VFSFile *stream, gchar const *format, va_list args)
{
    gchar *string;
    gint rv = g_vasprintf(&string, format, args);
    if (rv<0) return rv;
    rv = vfs_fputs(string, stream);
    free (string);
    return rv;
}

/**
 * Writes a formatted string to a VFS stream.
 *
 * @param stream A #VFSFile object representing the stream.
 * @param format A printf-style format string.
 * @param ... Optional list of arguments.
 * @return The amount of bytes written.
 */
gint vfs_fprintf(VFSFile *stream, gchar const *format, ...)
{
    va_list arg;
    gint rv;

    va_start(arg, format);
    rv = vfs_vfprintf(stream, format, arg);
    va_end(arg);

    return rv;
}

/**
 * Gets contents of the file into a buffer. Buffer of filesize bytes
 * is allocated by this function as necessary.
 *
 * @param filename URI of the file to read in.
 * @param buf Pointer to a pointer variable of buffer.
 * @param sz Pointer to gsize variable that will hold the amount of
 * read data e.g. filesize.
 */
void
vfs_file_get_contents(const gchar *filename, gchar **buf, gsize *size)
{
    VFSFile *fd;
    size_t filled_size = 0;
    size_t buf_size = 4096;
    gchar *ptr;
    
    fd = vfs_fopen(filename, "rb");

    if (fd == NULL)
        return;

    if ( vfs_fseek(fd, 0, SEEK_END) == 0) { // seeking supported by VFS backend
	    *size = vfs_ftell(fd);

	    *buf = g_new(gchar, *size);
    
	    if (*buf == NULL)
    		goto close_handle;

	    vfs_fseek(fd, 0, SEEK_SET);
	    vfs_fread(*buf, 1, *size, fd);

	    goto close_handle;
    }


    *buf = g_new(gchar, buf_size);
    
    if (*buf == NULL)
    	goto close_handle;

    ptr=*buf;
    while ( 1 ) {
	    size_t read_size = vfs_fread(ptr, 1, buf_size - filled_size, fd);
	    if ( read_size == 0 ) break;
	    
	    filled_size+=read_size;
	    ptr+=read_size;
	    
	    if ( filled_size == buf_size ) {
		    buf_size+=4096;
		    
		    *buf = g_realloc(*buf, buf_size);
		    
		    if ( *buf == NULL )
			goto close_handle;
			
		    ptr=*buf + filled_size;
	    }
    }

    *size = filled_size;
    
    close_handle:
    vfs_fclose(fd);    
}


/**
 * Reads an unsigned 16-bit Little Endian value from the stream
 * into native endian format.
 *
 * @param value Pointer to the variable to read the value into.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
gboolean vfs_fget_le16(guint16 *value, VFSFile *stream)
{
    guint16 tmp;
    if (vfs_fread(&tmp, sizeof(guint16), 1, stream) != 1)
        return FALSE;
    *value = GUINT16_FROM_LE(tmp);
    return TRUE;
}

/**
 * Reads an unsigned 32-bit Little Endian value from the stream into native endian format.
 *
 * @param value Pointer to the variable to read the value into.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
gboolean vfs_fget_le32(guint32 *value, VFSFile *stream)
{
    guint32 tmp;
    if (vfs_fread(&tmp, sizeof(guint32), 1, stream) != 1)
        return FALSE;
    *value = GUINT32_FROM_LE(tmp);
    return TRUE;
}

/**
 * Reads an unsigned 64-bit Little Endian value from the stream into native endian format.
 *
 * @param value Pointer to the variable to read the value into.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
gboolean vfs_fget_le64(guint64 *value, VFSFile *stream)
{
    guint64 tmp;
    if (vfs_fread(&tmp, sizeof(guint64), 1, stream) != 1)
        return FALSE;
    *value = GUINT64_FROM_LE(tmp);
    return TRUE;
}


/**
 * Reads an unsigned 16-bit Big Endian value from the stream into native endian format.
 *
 * @param value Pointer to the variable to read the value into.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
gboolean vfs_fget_be16(guint16 *value, VFSFile *stream)
{
    guint16 tmp;
    if (vfs_fread(&tmp, sizeof(guint16), 1, stream) != 1)
        return FALSE;
    *value = GUINT16_FROM_BE(tmp);
    return TRUE;
}

/**
 * Reads an unsigned 32-bit Big Endian value from the stream into native endian format.
 *
 * @param value Pointer to the variable to read the value into.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
gboolean vfs_fget_be32(guint32 *value, VFSFile *stream)
{
    guint32 tmp;
    if (vfs_fread(&tmp, sizeof(guint32), 1, stream) != 1)
        return FALSE;
    *value = GUINT32_FROM_BE(tmp);
    return TRUE;
}

/**
 * Reads an unsigned 64-bit Big Endian value from the stream into native endian format.
 *
 * @param value Pointer to the variable to read the value into.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
gboolean vfs_fget_be64(guint64 *value, VFSFile *stream)
{
    guint64 tmp;
    if (vfs_fread(&tmp, sizeof(guint64), 1, stream) != 1)
        return FALSE;
    *value = GUINT64_FROM_BE(tmp);
    return TRUE;
}

