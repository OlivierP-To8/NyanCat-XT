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
				unsigned char bp0=0, bp1=0, bp2=0, bp3=0, n=0;
				int j, reste;

				if (i!=h) printf(",");
				pos -= pitch/2;
				fseek(f, pos, SEEK_SET);
				for (j=0; j<w/2; j++)
				{
					unsigned char c = fgetc(f);
					unsigned char p0b0 = (c & 128) >> 7;
					unsigned char p0b1 = (c & 64) >> 6;
					unsigned char p0b2 = (c & 32) >> 5;
					unsigned char p0b3 = (c & 16) >> 4;
					unsigned char p1b0 = (c & 8) >> 3;
					unsigned char p1b1 = (c & 4) >> 2;
					unsigned char p1b2 = (c & 2) >> 1;
					unsigned char p1b3 = (c & 1);

					switch (n)
					{
					case 0:
						bp0 |= (p0b0 << 7);
						bp1 |= (p0b1 << 7);
						bp2 |= (p0b2 << 7);
						bp3 |= (p0b3 << 7);
						bp0 |= (p1b0 << 6);
						bp1 |= (p1b1 << 6);
						bp2 |= (p1b2 << 6);
						bp3 |= (p1b3 << 6);
						n = 2;
						break;
					case 1:
						break;
					case 2:
						bp0 |= (p0b0 << 5);
						bp1 |= (p0b1 << 5);
						bp2 |= (p0b2 << 5);
						bp3 |= (p0b3 << 5);
						bp0 |= (p1b0 << 4);
						bp1 |= (p1b1 << 4);
						bp2 |= (p1b2 << 4);
						bp3 |= (p1b3 << 4);
						n = 4;
						break;
					case 3:
						break;
					case 4:
						bp0 |= (p0b0 << 3);
						bp1 |= (p0b1 << 3);
						bp2 |= (p0b2 << 3);
						bp3 |= (p0b3 << 3);
						bp0 |= (p1b0 << 2);
						bp1 |= (p1b1 << 2);
						bp2 |= (p1b2 << 2);
						bp3 |= (p1b3 << 2);
						n = 6;
						break;
					case 5:
						break;
					case 6:
						bp0 |= (p0b0 << 1);
						bp1 |= (p0b1 << 1);
						bp2 |= (p0b2 << 1);
						bp3 |= (p0b3 << 1);
						bp0 |= (p1b0 << 0);
						bp1 |= (p1b1 << 0);
						bp2 |= (p1b2 << 0);
						bp3 |= (p1b3 << 0);
						n = 8;
						break;
					case 7:
						break;
					}
					if (n==8)
					{
						printf("0x%02x,", (int)bp3);
						printf("0x%02x,", (int)bp2);
						printf("0x%02x,", (int)bp1);
						printf("0x%02x,", (int)bp0);
						bp0 = bp1 = bp2 = bp3 = n = 0;
					}
				}
				printf(" ENDL,");
				reste = (i==1) ? 0 : 40 - w/8;
				printf("0x%02x\n\t", reste);
			}
			printf("\};\n");
			fclose(f);
		}
	else printf("impossible d'ouvrir %s\n", argv[1]);
	}
	return 0;
}
