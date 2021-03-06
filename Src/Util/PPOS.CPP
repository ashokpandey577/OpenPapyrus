// PPOS.CPP
// Copyright (c) A.Sobolev 1997-2001, 2002, 2008, 2014
//
// PROJECT PAPYRUS
// ������� ��� ������ � ������� ����� ����� ��������� ��
//
// ��������� ������:
//		PPOS a [/c "called cmd line"] file_templ
//			��������� ������������ ������ �� ������� file_templ
//			��� ��������� ���������� ��������� ������ "called cmd line"
//          � ���������� ��������� ������ ��������� %s ���������
//			�� ��� ������������� �����. ���� ���������� ��������� ������
//			�� �������, �� ������ ������������ ������ ��������� �� �������.
//		������: PPOS a /c "pkzip pps_in %s" g:\ppy\in\*.pps
//
//		PPOS s [/c "called cmd line"] file_templ
//          ���������� ����������� ������, � ��� ���� ��������, ���
//          ������������ ������ �� �����, ������� �� �������� ���
//          ��������������.
//
//		PPOS [v] file_name
//			��������� ����������� ���������� ����� file_name
//

#include <pp.h>

#include <direct.h>
#include <locale.h>

unsigned _stklen = 0x4000U;

int PPErrCode = 0;

class DirStack {
public:
	SLAPI DirStack()
	{
		stack = 0;
	}
	SLAPI ~DirStack()
	{
		delete stack;
	}
	int SLAPI Push();
	int SLAPI Pop();
private:
	struct Path {
		int  drv;
		char dir[MAXDIR];
	};
	SStack * stack;
};

int SLAPI DirStack::Push()
{
	Path p;
	p.drv = getdisk();
	p.dir[0] = '\\';
	_getcwd(p.dir+1, sizeof(p.dir)-2);
	if(stack == 0) {
		stack = new SStack(sizeof(Path));
		if(stack == 0)
			return 0;
	}
	return stack->push(&p);
}

int SLAPI DirStack::Pop()
{
	Path p;
	if(stack && stack->pop(&p)) {
		_chdrive(p.drv);
		_chdir(p.dir);
		return 1;
	}
	return 0;
}

char * help =

"PROJECT PAPYRUS\n"
"�⨫�� ��� ࠡ��� � 䠩���� �裡 ����� ࠧ������ ��\n\n"

"��������� ��ப�:\n"
"\tPPOS a [/c \"called cmd line\"] <file_templ>\n"
"\t\t��娢��� ��ࠡ�⠭��� 䠩��� �� 蠡���� file_templ\n"
"\t\t��� ��娢�樨 ��뢠���� ��������� ��ப� \"called cmd line\"\n"
"\t\t� ��뢠���� ��������� ��ப� �����ப� %%s ���������\n"
"\t\t�� ��� ��娢��㥬��� 䠩��. �᫨ ��뢠���� ��������� ��ப�\n"
"\t\t�� 㪠����, � ᯨ᮪ ��ࠡ�⠭��� 䠩��� �뢮����� �� ���᮫�.\n"
"\t\t�ਬ��: PPOS a /c \"pkzip pps_in %%s\" g:\\ppy\\in\\*.pps\n\n"

"\tPPOS s [/c \"called cmd line\"] file_templ\n"
"\t\t�������筮 �।��饬� ����, � ⮩ ���� ࠧ��楩, ��\n"
"\t\t��ॡ������ ⮫쪮 � 䠩��, ����� �� ����祭� ���\n"
"\t\t�ᯮ�짮�����.\n\n"

"\tPPOS v file_name\n"
"\t\t��ᬮ���� ᮤ�ন��� 䠩�� file_name\n";

void err_cmd_line()
{
	printf("�訡�� � ��������� ��ப�\n\n");
	printf(help);
	exit(-1);
}

// Prototypes
static void archive(char *, char *, int);
static void view(char * fname);

void main(int argc, char * argv[])
{
	setlocale(LC_ALL, "rus");
	DirStack ds;
	if(argc < 2) {
		err_cmd_line();
	}
	if(stricmp(argv[1], "a") == 0 || stricmp(argv[1], "s") == 0) {
		int a = (stricmp(argv[1], "a") == 0) ? 1 : -1;
		if(argc < 3)
			err_cmd_line();
		char * called_cmd_line = 0;
		int next = 2;
		if(stricmp(argv[2], "/c") == 0) {
			called_cmd_line = argv[3];
			next = 4;
		}
		ds.Push();
		for(int i = next; i < argc; i++) {
			archive(argv[i], called_cmd_line, a);
		}
		ds.Pop();
	}
	else if(stricmp(argv[1], "v") == 0) {
		if(argc < 3)
			err_cmd_line();
		view(argv[2]);
	}
	else if(stricmp(argv[1], "/?") == 0) {
		printf(help);
	}
	else
		view(argv[1]);
}

static void archive(char * fname, char * cmd_line, int a)
{
	char cmd[512];
	char * p, * d = cmd, * s = cmd_line;
	if(cmd_line) {
		while((p = strstr(s, "%s")) != 0) {
			while(s != p)
				*d++ = *s++;
			d = stpcpy(d, fname);
			s += 2;
		}
		while(*s)
			*d++ = *s++;
		*d = 0;
	}
	FILE * f = fopen(fname, "r+b");
	if(f) {
		PPObjectTransmit::Header hdr;
		if(fread(&hdr, sizeof(hdr), 1, f) < 1) {
			printf("�訡�� �⥭�� 䠩�� %s\n", fname);
			fclose(f);
		}
		else {
			fclose(f);
			if(hdr.Magic == 0x534F5050L && (hdr.PacketType == PPOT_OBJ ||
				hdr.PacketType == PPOT_ACK)) {
				if((a > 0 && hdr.Flags & PPOTF_ACK) ||
					(a < 0 && !(hdr.Flags & PPOTF_ACK)) || a == 0) {
					if(cmd_line) {
						if(system(cmd) != 0) {
							printf("�訡�� ����᪠ ��������� ��ப� \n\t\"%s\"\n", cmd);
							exit(-1);
						}
					}
					else
						printf("%s\n", fname);
				}
			}
		}
	}
}

static int print_obj_list(FILE * pStream)
{
	PPObjectTransmit::Header hdr;
	fseek(pStream, 0L, SEEK_SET);
	fread(&hdr, sizeof(hdr), 1, pStream);
	fseek(pStream, hdr.IndexOffs, SEEK_SET);

	printf("\n");
	//      12345678901234567890123456789012345678901234567890123456789012345678901234567890
	printf("     ObjType     ForeignID    CommonID   Position  ��� �����  �६� ����� �����\n");
	printf("--------------------------------------------------------------------------------\n");
	for(uint i = 0; i < hdr.IndexCount; i++) {
		char dt_buf[32], tm_buf[32], f_buf[32];
		PPObjectTransmit::IndexItem idx_item;
		fread(&idx_item, sizeof(idx_item), 1, pStream);

		memset(f_buf, 0, sizeof(f_buf));
		size_t fbp = 0;
		if(idx_item.Flags & PPObjPack::fForceUpdate)
			f_buf[fbp++] = 'U';
		// @v8.2.3 {
		if(idx_item.Flags & PPObjPack::fRecover)
			f_buf[fbp++] = 'R';
		// } @v8.2.3
		printf("%12ld%14ld%12ld%10lXH%12s%13s%6s\n",
			idx_item.ObjType, idx_item.ObjID, idx_item.ObjCommID,
			idx_item.ObjOffs,
			datefmt(&idx_item.Mod.d, DATF_DMY|DATF_CENTURY, dt_buf),
			timefmt(idx_item.Mod.t, TIMF_HMS, tm_buf),
			f_buf
			);
	}
	printf("--------------------------------------------------------------------------------\n");
	return 1;
}

static void view(char * fname)
{
	char * p;
	FILE * f = fopen(fname, "rb");
	if(f == 0) {
		printf("�訡�� ������ 䠩�� %s\n", fname);
	}
	else {
		PPObjectTransmit::Header hdr;
		if(fread(&hdr, sizeof(hdr), 1, f) < 1) {
			printf("�訡�� �⥭�� 䠩�� %s\n", fname);
		}
		else if(hdr.Magic != 0x534F5050L) {
			printf("%s �� ���� 䠩��� �裡 ࠧ�����\n", fname);
		}
		else {
			int major, minor, rel;
			printf("����: %s\n", fname);
			if(hdr.PacketType == PPOT_OBJ)
				p = "PPOT_OBJ";
			else if(hdr.PacketType == PPOT_ACK)
				p = "PPOT_ACK";
			else
				p = "Unknown";
			printf("��� �����  : %s\n", p);
			printf("���筨�    : %ld\n", hdr.DBID);
			printf("�����     : %ld\n", hdr.DestDBID);
			p = (hdr.Flags & PPOTF_ACK) ? "��" : "���";
			printf("�ᯮ�짮��� : %s\n", p);
			p = (hdr.Flags & PPOTF_ARC) ? "��" : "���";
			printf("��������    : %s\n", p);
		    hdr.SwVer.Get(&major, &minor, &rel);
			printf("�����      : %d.%d.%d\n", major, minor, rel);
			if(hdr.PacketType == PPOT_OBJ) {
				printf("������⢮ ��ꥪ⮢: %ld\n", hdr.IndexCount);
				printf("���饭�� �� ������: 0x%lX\n", hdr.IndexOffs);
				print_obj_list(f);
			}
		}
	}
	if(f)
		fclose(f);
}

