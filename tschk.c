/*
 * TSデータのチェックプログラム
 */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include "bit_access.h"
#include "message_trace.h"

#define PACKSIZE 188

#if 1
#define R "\033[31m"
#define Y "\033[33m"
#define B "\033[34m"
#define P "\033[35m"
#define C "\033[m"
#else
#define R ""
#define Y ""
#define B ""
#define P ""
#define C ""
#endif

typedef struct _ts_data {
	int payload_unit_start_indicator;
	int pid;
	int adaptation_field_control;
	int continuity_counter;
	int adaptation_field_length;
	int PCR_flag;
	int OPCR_flag;
	int splicing_point_flag;
	int transport_private_data_flag;
	int adaptation_field_extension_flag;
	long long int program_clock_reference_base;
	int program_clock_reference_extension;
	long long int original_program_clock_reference_base;
	int original_program_clock_reference_extension;
	int PTS_DTS_flags;
	long long int pts;
	long long int dts;
} ts_data_t;

#define MAX_PROG 8
typedef struct program_info {
	int program_number;
	int pid;
	enum {
		PI_PROGRAM_MAP_PID=1,
		PI_NETWORK_PID=2,
		PI_PCR_PID=4,
		PI_PES_PID=8
	}	type;
	enum {	/* value of "H.222 Table 2-34 ? Stream type assignments" */
		H222_STREAM_TYPE_AVC=0x1B,
		H222_STREAM_TYPE_AAC=0x0F,
	}	stream_type;
}program_info_t;

int cmp_program_info(const void * p1, const void * p2)
{
	return ((program_info_t*)p1)->pid-((program_info_t*)p2)->pid;
}
static int64_t read_temp;
#define BIT(len) (bit_read64(buff,&pos,&bit,&read_temp,len)>0?read_temp:-1)

static inline program_info_t * search_program(program_info_t pi[], int * pi_cnt, int pid)
{
	program_info_t searchinfo={0,pid,0}, * prog;
	if((prog=bsearch(&searchinfo,pi,*pi_cnt,sizeof(*pi),cmp_program_info))==NULL)
	{
		prog=&pi[(*pi_cnt)++];
		*prog=searchinfo;
		qsort(pi,*pi_cnt,sizeof(*pi),cmp_program_info);
		prog=bsearch(&searchinfo,pi,*pi_cnt,sizeof(*pi),cmp_program_info);
	}
	return prog;
}

int main(int argc, char * argv[])
{
	int findex=0;
	FILE * fp = NULL;
	program_info_t pi[MAX_PROG];
	int pi_cnt=0;
	long long int last_pcr=0;

	while(1)
	{
		program_info_t * prog = NULL;
		ts_data_t ts={0};
		unsigned char buff[PACKSIZE];
		int pos=0,bit=0;
		char pcr_str[64]="             ";
		if(!fp || fread(buff,1,PACKSIZE,fp)<PACKSIZE)
		{
			if(fp)
			{
				fclose(fp);
				fp=NULL;
			}
			if(++findex>=argc)
			{
				break;
			}
			fp = fopen(argv[findex],"r");
			MESSAGE("open %s %s\n",argv[findex],fp?"success":"faild");
			continue;
		}
		while(buff[0]!=0x47)
		{
			int i;
			for(i=0; i<PACKSIZE && buff[i]!=0x47; i++);
			if(i<PACKSIZE)
			{
				memmove(buff+0, buff+i, PACKSIZE-i);
				if(fread(buff+i,1,PACKSIZE-i,fp)<PACKSIZE-i)
					goto END;
			}
		}
		/*sync_byte*/BIT(8);
		/*transport_error_indicator*/BIT(1);
		ts.payload_unit_start_indicator=BIT(1);
		/*transport_priority*/BIT(1);
		ts.pid=BIT(13);
		/*transport_scrambling_control*/BIT(2);
		ts.adaptation_field_control=BIT(2);
		ts.continuity_counter=BIT(4);
		if(ts.adaptation_field_control&0x02)
		{
			int endpos;
			ts.adaptation_field_length=BIT(8);
			endpos=pos+ts.adaptation_field_length;
			if(ts.adaptation_field_length>0)
			{
				/*discontinuity_indicator*/BIT(1);
				/*random_access_indicator*/BIT(1);
				/*elementary_stream_priority_indicator*/BIT(1);
				ts.PCR_flag=BIT(1);
				ts.OPCR_flag=BIT(1);
				ts.splicing_point_flag=BIT(1);
				ts.transport_private_data_flag=BIT(1);
				ts.adaptation_field_extension_flag=BIT(1);
				if(ts.PCR_flag)
				{
					ts.program_clock_reference_base=BIT(33);
					/*reserved*/BIT(6);
					ts.program_clock_reference_extension=BIT(9);
				}
				if(ts.OPCR_flag)
				{
					ts.original_program_clock_reference_base=BIT(33);
					/*reserved*/BIT(6);
					ts.original_program_clock_reference_extension=BIT(9);
				}
				if(ts.splicing_point_flag)
				{
					/*splice_countdown*/BIT(8);
				}
				if(ts.transport_private_data_flag)
				{
					int transport_private_data_length=BIT(8);
					int i;
					for(i=0; i<transport_private_data_length; i++)
						/*private_data_byte*/BIT(8);
				}
				if(ts.adaptation_field_extension_flag)
				{
					int adaptation_field_extension_length,endpos;
					int ltw_flag,piecewise_rate_flag,seamless_splice_flag;
					adaptation_field_extension_length=BIT(8);
					endpos=pos+adaptation_field_extension_length;
					ltw_flag=BIT(1);
					piecewise_rate_flag=BIT(1);
					seamless_splice_flag=BIT(1);
					/*reserved*/BIT(5);
					if(ltw_flag)
					{
						/*ltw_valid_flag*/BIT(1);
						/*ltw_offset*/BIT(15);
					}
					if(piecewise_rate_flag)
					{
						/*reserved*/BIT(2);
						/*piecewise_rate*/BIT(22);
					}
					if(seamless_splice_flag)
					{
						/*splice_type*/BIT(4);
						/*DTS_next_AU*/BIT(3);
						/*marker_bit*/BIT(1);
						/*DTS_next_AU*/BIT(15);
						/*marker_bit*/BIT(1);
						/*DTS_next_AU*/BIT(15);
						/*marker_bit*/BIT(1);
					}
					assert(bit==0);
					while(pos<endpos)
						/*reserved*/BIT(8);
				}
			}
			assert(bit==0);
			while(pos<endpos)
				/*stuffing_byte*/BIT(8);
		}
		if(pi_cnt)
			prog = search_program(pi, &pi_cnt, ts.pid);
		if(prog && prog->type==PI_PROGRAM_MAP_PID)
		{	/* program map */
			int i;
			int table_id,section_length;
			int PCR_PID;
			int program_info_length;
			int section_end;
			int program_number;
			int section_number;
			int last_section_number;
			char es_str[1024]="";
			/* テーブル先頭の1バイトを読み捨て(テーブルサイズもしくは0) */BIT(8);
			table_id=BIT(8);
			if(table_id!=2)
				continue;	/* PMT以外は無視 */
			/* section_syntax_indicator */BIT(1);
			/* '0' */BIT(1);
			/* reserved */BIT(2);
			section_length=BIT(12);
			section_end=pos+section_length-4;	/* 4はCRC_32のサイズ */
			program_number=BIT(16);
			/* reserved */BIT(2);
			/* version_number */BIT(5);
			/* current_next_indicator */BIT(1);
			section_number=BIT(8);
			last_section_number=BIT(8);
			/* reserved */BIT(3);
			PCR_PID=BIT(13);
			{
				program_info_t * pcr;
				if(pi_cnt)
					pcr=search_program(pi, &pi_cnt, PCR_PID);
				pcr->program_number=prog->program_number;
				pcr->type|=PI_PCR_PID;
			}
			/* reserved */BIT(4);
			program_info_length=BIT(12);
			for(i=0; i<program_info_length; i++)
			{
				/* descriptor() */BIT(8);
			}
			for(;pos<section_end;)
			{
				int stream_type;
				int elementary_PID;
				int ES_info_length;
				stream_type=BIT(8);
				/* reserved */BIT(3);
				elementary_PID=BIT(13);
				/* reserved */BIT(4);
				ES_info_length=BIT(12);
				for(i=0; i<ES_info_length && pos<=section_end; i++)
				{
					/* descriptor() */BIT(8);
				}
				program_info_t * pes;
				if(pi_cnt)
					pes=search_program(pi, &pi_cnt, elementary_PID);
				pes->program_number=prog->program_number;
				pes->type|=PI_PES_PID;
				pes->stream_type=stream_type;
				sprintf(es_str+strlen(es_str),"(es pid=%04x type=%04x)",elementary_PID,stream_type);
			}
			MESSAGE(Y"PMT pid=%04x pcr=%04x af=%d cc=%02d %s\n"C,
					ts.pid,PCR_PID,ts.adaptation_field_control,ts.continuity_counter,es_str);
			/* CRC_32 */BIT(32);
			continue;
		}
		if(prog && (prog->type&PI_PCR_PID) && ts.PCR_flag)
		{
			last_pcr=ts.program_clock_reference_base;
			sprintf(pcr_str,P"PCR:% 9lld"C,ts.program_clock_reference_base);
		}
		if(ts.adaptation_field_control&0x01)
		{
			if(ts.pid==0)
			{	/* Program Association Table */
				int endpos;
				int table_id,section_length;
				const program_info_t ini_pi={0};
				int sub_pid;
				///* 先頭のときはテーブルを初期化 */
				//if(ts.payload_unit_start_indicator)
				//	pi_cnt=0;
				/* テーブル先頭の1バイトを読み捨て(テーブルサイズもしくは0) */BIT(8);
				table_id=BIT(8);
				if(table_id!=0)
					continue;	/* PAT以外は無視 */
				/* section_syntax_indicator */BIT(1);
				/* () */BIT(1);
				/* reserved */BIT(2);
				section_length=BIT(12);
				endpos=pos+section_length;
				/* transport_stream_id */BIT(16);
				/* reserved */BIT(2);
				/* version_number */BIT(5);
				/* current_next_indicator */BIT(1);
				/* section_number */BIT(8);
				/* last_section_number */BIT(8);
				if(pi_cnt==0)
				{
					while(pos<endpos-4/*CRC_32を差し引く*/)
					{
						pi[pi_cnt]=ini_pi;
						pi[pi_cnt].program_number=BIT(16);
						/* reserved */BIT(3);
						pi[pi_cnt].pid=sub_pid=BIT(13);
						if(pi[pi_cnt].program_number==0)
							pi[pi_cnt].type=PI_NETWORK_PID;
						else
							pi[pi_cnt].type=PI_PROGRAM_MAP_PID;
						pi_cnt++;
					}
					qsort(pi,pi_cnt,sizeof(*pi),cmp_program_info);
				}
				/* CRC_32 */BIT(32);
				{
					program_info_t * pmt;
					pmt=search_program(pi, &pi_cnt, sub_pid);
					MESSAGE(B"PAT pid=%04x af=%d cc=%02d (pn:%d pid=%04x)\n"C,
							ts.pid, ts.adaptation_field_control, ts.continuity_counter,
							pmt->program_number, pmt->pid);
				}
			}
			else if(prog && prog->type)
			{
				if(ts.payload_unit_start_indicator)
				{
					char errindicator[2][8]={R,R};
					int packet_start_code_prefix;
					program_info_t * pes;
					packet_start_code_prefix=(BIT(8)<<16)|(BIT(8)<<8)|BIT(8);
					if(packet_start_code_prefix!=1)
					{
						MESSAGE("    pid=%04x type=%d start code prefix != 0x000001, value=%06d\n",ts.pid,prog->type,packet_start_code_prefix);
						continue;
					}
					if((pes=search_program(pi, &pi_cnt, ts.pid))==NULL)
					{
						MESSAGE("unknown program id %d\n",ts.pid);
						continue;
					}
					ts.pts=0;
					ts.dts =0;
					/* stream_id */BIT(8);
					/* PES_packet_length */BIT(16);
					/* '10' */BIT(2);
					/* PES_scrambling_control */BIT(2);
					/* PES_priority */BIT(1);
					/* data_alignment_indicator */BIT(1);
					/* copyright */BIT(1);
					/* original_or_copy */BIT(1);
					ts.PTS_DTS_flags=BIT(2);
					/* ESCR_flag */BIT(1);
					/* ES_rate_flag */BIT(1);
					/* DSM_trick_mode_flag */BIT(1);
					/* additional_copy_info_flag */BIT(1);
					/* PES_CRC_flag */BIT(1);
					/* PES_extension_flag */BIT(1);
					/* PES_header_data_length */BIT(8);
					if(ts.PTS_DTS_flags&2)
					{
						/* '0010' or '0011' */BIT(4);
						ts.pts|=BIT(3)<<30;
						/* marker_bit */BIT(1);
						ts.pts|=BIT(15)<<15;
						/* marker_bit */BIT(1);
						ts.pts|=BIT(15);
						/* marker_bit */BIT(1);
					}
					if(ts.PTS_DTS_flags&1)
					{
						/* '0001' */BIT(4);
						ts.dts|=BIT(3)<<30;
						/* marker_bit */BIT(1);
						ts.dts|=BIT(15)<<15;
						/* marker_bit */BIT(1);
						ts.dts|=BIT(15);
						/* marker_bit */BIT(1);
					}
					if(last_pcr<ts.pts||ts.pts==0)
						errindicator[0][0]='\0';
					if(last_pcr<ts.dts||ts.dts==0)
						errindicator[1][0]='\0';
					MESSAGE("PES pid=%04x af=%d cc=%02d %s PTS:%s% 9lld DTS:%s% 9lld %s\n"C,
							ts.pid,ts.adaptation_field_control,ts.continuity_counter,
							pcr_str,
							errindicator[0],ts.pts,errindicator[1],ts.dts,
							pes->stream_type==H222_STREAM_TYPE_AVC?"avc":
							pes->stream_type==H222_STREAM_TYPE_AAC?"aac":"");
				}
				else
					MESSAGE("PES pid=%04x af=%d cc=%02d %s \n",ts.pid,ts.adaptation_field_control,ts.continuity_counter,pcr_str);
			}
			else
			{
				MESSAGE("??? pid=%04x %s\n",ts.pid,pcr_str);
			}
		}


	}
#undef BIT
END:
	if(fp)
		fclose(fp);

	return 0;
}

