/*******************************************************************************
* Rapide programme pour transformer un bmp 16 couleurs en data                 *
********************************************************************************
* Author  : OlivierP                                                           *
* Date    : march & july 2012                                                  *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	if (argc < 3)
		printf("usage : %s fichier.bmp nom\n", argv[0]);
	else
	{
		FILE *f = fopen(argv[1], "rb");
		if (f != NULL)
		{
			char w, h;
			int pitch, pos, i;

			printf("\nunsigned char %s\[\] = \{\n\t ", argv[2]);

			fseek(f, 18, SEEK_SET);
			w = fgetc(f);

			fseek(f, 22, SEEK_SET);
			h = fgetc(f);

			pitch = (w%8) ? ((w>>3)+1)<<3 : w;

			fseek(f, 0, SEEK_END);
			pos = ftell(f);

			for (i=h; i>0; i--)
			{
				int j, reste;

				if (i!=h) printf(",");
				pos -= pitch/2;
				fseek(f, pos, SEEK_SET);
				for (j=0; j<w/2; j++)
				{
					unsigned char c = fgetc(f);
					unsigned char b0 = (c & 0xf0) >> 4;
					unsigned char b1 = (c & 0x0f);

					printf("0x%02x,", (int)b0);
					printf("0x%02x,", (int)b1);
				}
				printf(" ENDL,");
				reste = (i==1) ? 0 : 80 - w/4;
				printf("0x%02x\n\t", reste);
			}
			printf("\};\n");
			fclose(f);
		}
		else printf("impossible d'ouvrir %s\n", argv[1]);
	}
	return 0;
}
