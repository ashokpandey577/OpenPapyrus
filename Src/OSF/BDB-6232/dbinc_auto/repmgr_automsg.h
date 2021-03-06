/* Do not edit: automatically built by gen_msg.awk. */

#ifndef	__repmgr_AUTOMSG_H
#define	__repmgr_AUTOMSG_H

/*
 * Message sizes are simply the sum of field sizes (not
 * counting variable size parts, when DBTs are present),
 * and may be different from struct sizes due to padding.
 */
#define	__REPMGR_HANDSHAKE_SIZE	12
typedef struct ___repmgr_handshake_args {
	uint16	port;
	uint16	alignment;
	uint32	ack_policy;
	uint32	flags;
} __repmgr_handshake_args;

#define	__REPMGR_V3HANDSHAKE_SIZE	10
typedef struct ___repmgr_v3handshake_args {
	uint16	port;
	uint32	priority;
	uint32	flags;
} __repmgr_v3handshake_args;

#define	__REPMGR_V2HANDSHAKE_SIZE	6
typedef struct ___repmgr_v2handshake_args {
	uint16	port;
	uint32	priority;
} __repmgr_v2handshake_args;

#define	__REPMGR_PARM_REFRESH_SIZE	8
typedef struct ___repmgr_parm_refresh_args {
	uint32	ack_policy;
	uint32	flags;
} __repmgr_parm_refresh_args;

#define	__REPMGR_PERMLSN_SIZE	12
typedef struct ___repmgr_permlsn_args {
	uint32	generation;
	DB_LSN		lsn;
} __repmgr_permlsn_args;

#define	__REPMGR_VERSION_PROPOSAL_SIZE	8
typedef struct ___repmgr_version_proposal_args {
	uint32	min;
	uint32	max;
} __repmgr_version_proposal_args;

#define	__REPMGR_VERSION_CONFIRMATION_SIZE	4
typedef struct ___repmgr_version_confirmation_args {
	uint32	version;
} __repmgr_version_confirmation_args;

#define	__REPMGR_MSG_HDR_SIZE	9
typedef struct ___repmgr_msg_hdr_args {
	uint8	type;
	uint32	word1;
	uint32	word2;
} __repmgr_msg_hdr_args;

#define	__REPMGR_MSG_METADATA_SIZE	12
typedef struct ___repmgr_msg_metadata_args {
	uint32	tag;
	uint32	limit;
	uint32	flags;
} __repmgr_msg_metadata_args;

#define	__REPMGR_MEMBERSHIP_KEY_SIZE	6
typedef struct ___repmgr_membership_key_args {
	DBT		host;
	uint16	port;
} __repmgr_membership_key_args;

#define	__REPMGR_MEMBERSHIP_DATA_SIZE	8
typedef struct ___repmgr_membership_data_args {
	uint32	status;
	uint32	flags;
} __repmgr_membership_data_args;

#define	__REPMGR_V4MEMBERSHIP_DATA_SIZE	4
typedef struct ___repmgr_v4membership_data_args {
	uint32	flags;
} __repmgr_v4membership_data_args;

#define	__REPMGR_MEMBER_METADATA_SIZE	8
typedef struct ___repmgr_member_metadata_args {
	uint32	format;
	uint32	version;
} __repmgr_member_metadata_args;

#define	__REPMGR_GM_FWD_SIZE	10
typedef struct ___repmgr_gm_fwd_args {
	DBT		host;
	uint16	port;
	uint32	gen;
} __repmgr_gm_fwd_args;

#define	__REPMGR_MEMBR_VERS_SIZE	8
typedef struct ___repmgr_membr_vers_args {
	uint32	version;
	uint32	gen;
} __repmgr_membr_vers_args;

#define	__REPMGR_SITE_INFO_SIZE	14
typedef struct ___repmgr_site_info_args {
	DBT		host;
	uint16	port;
	uint32	status;
	uint32	flags;
} __repmgr_site_info_args;

#define	__REPMGR_V4SITE_INFO_SIZE	10
typedef struct ___repmgr_v4site_info_args {
	DBT		host;
	uint16	port;
	uint32	flags;
} __repmgr_v4site_info_args;

#define	__REPMGR_CONNECT_REJECT_SIZE	12
typedef struct ___repmgr_connect_reject_args {
	uint32	version;
	uint32	gen;
	uint32	status;
} __repmgr_connect_reject_args;

#define	__REPMGR_V4CONNECT_REJECT_SIZE	8
typedef struct ___repmgr_v4connect_reject_args {
	uint32	version;
	uint32	gen;
} __repmgr_v4connect_reject_args;

#define	__REPMGR_LSNHIST_MATCH_SIZE	24
typedef struct ___repmgr_lsnhist_match_args {
	DB_LSN		lsn;
	uint32	hist_sec;
	uint32	hist_nsec;
	DB_LSN		next_gen_lsn;
} __repmgr_lsnhist_match_args;

#define	__REPMGR_MAXMSG_SIZE	24
#endif
