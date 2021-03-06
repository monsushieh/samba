/*
 * Samba Unix/Linux SMB client library
 * Distributed SMB/CIFS Server Management Utility
 * Nltest netlogon testing tool
 *
 * Copyright (C) Guenther Deschner 2009
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netapi.h>

#include "common.h"

enum {
	OPT_SERVER = 1,
	OPT_DBFLAG,
	OPT_SC_QUERY,
	OPT_SC_RESET,
	OPT_SC_VERIFY,
	OPT_SC_CHANGE_PWD,
	OPT_DSGETDC,
	OPT_PDC,
	OPT_DS,
	OPT_DSP,
	OPT_GC,
	OPT_KDC,
	OPT_TIMESERV,
	OPT_GTIMESERV,
	OPT_WS,
	OPT_NETBIOS,
	OPT_DNS,
	OPT_IP,
	OPT_FORCE,
	OPT_WRITABLE,
	OPT_AVOIDSELF,
	OPT_LDAPONLY,
	OPT_BACKG,
	OPT_DS_6,
	OPT_TRY_NEXT_CLOSEST_SITE,
	OPT_SITE,
	OPT_ACCOUNT,
	OPT_RET_DNS,
	OPT_RET_NETBIOS,
	OPT_DSREGDNS
};

/****************************************************************
****************************************************************/

static void print_netlogon_info_result(uint32_t level,
				       uint8_t *buffer)
{
	struct NETLOGON_INFO_1 *i1 = NULL;
	struct NETLOGON_INFO_2 *i2 = NULL;
	struct NETLOGON_INFO_3 *i3 = NULL;
	struct NETLOGON_INFO_4 *i4 = NULL;

	if (!buffer) {
		return;
	}

	switch (level) {
	case 1:
		i1 = (struct NETLOGON_INFO_1 *)buffer;

		printf("Flags: %x\n", i1->netlog1_flags);
		printf("Connection Status Status = %d 0x%x %s\n",
			i1->netlog1_pdc_connection_status,
			i1->netlog1_pdc_connection_status,
			libnetapi_errstr(i1->netlog1_pdc_connection_status));

		break;
	case 2:
		i2 = (struct NETLOGON_INFO_2 *)buffer;

		printf("Flags: %x\n", i2->netlog2_flags);
		printf("Trusted DC Name %s\n", i2->netlog2_trusted_dc_name);
		printf("Trusted DC Connection Status Status = %d 0x%x %s\n",
			i2->netlog2_tc_connection_status,
			i2->netlog2_tc_connection_status,
			libnetapi_errstr(i2->netlog2_tc_connection_status));
		printf("Trust Verification Status Status = %d 0x%x %s\n",
			i2->netlog2_pdc_connection_status,
			i2->netlog2_pdc_connection_status,
			libnetapi_errstr(i2->netlog2_pdc_connection_status));

		break;
	case 3:
		i3 = (struct NETLOGON_INFO_3 *)buffer;

		printf("Flags: %x\n", i3->netlog1_flags);
		printf("Logon Attempts: %d\n", i3->netlog3_logon_attempts);

		break;
	case 4:
		i4 = (struct NETLOGON_INFO_4 *)buffer;

		printf("Trusted DC Name %s\n", i4->netlog4_trusted_dc_name);
		printf("Trusted Domain Name %s\n", i4->netlog4_trusted_domain_name);

		break;
	default:
		break;
	}
}

/****************************************************************
****************************************************************/

static void print_dc_info_flags(uint32_t flags)
{
	if (flags & DS_PDC_FLAG)
		printf("PDC ");
	if (flags & DS_GC_FLAG)
		printf("GC ");
	if (flags & DS_DS_FLAG)
		printf("DS ");
	if (flags & DS_LDAP_FLAG)
		printf("LDAP ");
	if (flags & DS_KDC_FLAG)
		printf("KDC ");
	if (flags & DS_TIMESERV_FLAG)
		printf("TIMESERV ");
	if (flags & DS_GOOD_TIMESERV_FLAG)
		printf("GTIMESERV ");
	if (flags & DS_WRITABLE_FLAG)
		printf("WRITABLE ");
	if (flags & DS_DNS_FOREST_FLAG)
		printf("DNS_FOREST ");
	if (flags & DS_CLOSEST_FLAG)
		printf("CLOSE_SITE ");
	if (flags & DS_FULL_SECRET_DOMAIN_6_FLAG)
		printf("FULL_SECRET ");
	if (flags & DS_WS_FLAG)
		printf("WS ");
	if (flags & DS_DS_8_FLAG)
		printf("DS_8 ");
	printf("\n");
}

/****************************************************************
****************************************************************/

static void print_dc_info(struct DOMAIN_CONTROLLER_INFO *dc_info)
{
	if (dc_info->flags) {
		printf("           DC: %s\n", dc_info->domain_controller_name);
		printf("      Address: %s\n", dc_info->domain_controller_address);
/*		printf("     Dom Guid: %s\n", X(domain_guid)); */
		printf("     Dom Name: %s\n", dc_info->domain_name);
		printf("  Forest Name: %s\n", dc_info->dns_forest_name);
		printf(" Dc Site Name: %s\n", dc_info->dc_site_name);
		printf("Our Site Name: %s\n", dc_info->client_site_name);
		printf("        Flags: ");
		print_dc_info_flags(dc_info->flags);
	} else {
		printf("           DC: %s\n", dc_info->domain_controller_name);
		printf("      Address: %s\n", dc_info->domain_controller_address);
		printf("     Dom Name: %s\n", dc_info->domain_name);
	}
}

/****************************************************************
****************************************************************/

int main(int argc, const char **argv)
{
	int opt;
	NET_API_STATUS status;
	struct libnetapi_ctx *ctx = NULL;
	char *opt_server = NULL;
	char *opt_domain = NULL;
	int opt_dbflag = 0;
	int opt_pdc = 0;
	int opt_ds = 0;
	int opt_dsp = 0;
	int opt_gc = 0;
	int opt_kdc = 0;
	int opt_timeserv = 0;
	int opt_gtimeserv = 0;
	int opt_ws = 0;
	int opt_netbios = 0;
	int opt_dns = 0;
	int opt_ip = 0;
	int opt_force = 0;
	int opt_writable = 0;
	int opt_avoidself = 0;
	int opt_ldaponly = 0;
	int opt_backg = 0;
	int opt_ds_6 = 0;
	int opt_try_next_closest_site = 0;
	char *opt_site = NULL;
	char *opt_account = NULL;
	int opt_ret_dns = 0;
	int opt_ret_netbios = 0;
	int opt_dsregdns = 0;
	uint32_t query_level = 0;
	uint8_t *buffer = NULL;
	uint32_t flags = 0;
	struct DOMAIN_CONTROLLER_INFO *dc_info = NULL;

	poptContext pc;
	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{
			.longName   = "server",
			.shortName  = 0,
			.argInfo    = POPT_ARG_STRING,
			.arg        = &opt_server,
			.val        = OPT_SERVER,
			.descrip    = "Servername",
			.argDescrip = "SERVER",
		},
		{
			.longName   = "dbflag",
			.shortName  = 0,
			.argInfo    = POPT_ARG_INT,
			.arg        = &opt_dbflag,
			.val        = OPT_DBFLAG,
			.descrip    = "New Debug Flag",
			.argDescrip = "HEXFLAGS",
		},
		{
			.longName   = "sc_query",
			.shortName  = 0,
			.argInfo    = POPT_ARG_STRING,
			.arg        = &opt_domain,
			.val        = OPT_SC_QUERY,
			.descrip    = "Query secure channel for domain on server",
			.argDescrip = "DOMAIN",
		},
		{
			.longName   = "sc_reset",
			.shortName  = 0,
			.argInfo    = POPT_ARG_STRING,
			.arg        = &opt_domain,
			.val        = OPT_SC_RESET,
			.descrip    = "Reset secure channel for domain on server to dcname",
			.argDescrip = "DOMAIN",
		},
		{
			.longName   = "sc_verify",
			.shortName  = 0,
			.argInfo    = POPT_ARG_STRING,
			.arg        = &opt_domain,
			.val        = OPT_SC_VERIFY,
			.descrip    = "Verify secure channel for domain on server",
			.argDescrip = "DOMAIN",
		},
		{
			.longName   = "sc_change_pwd",
			.shortName  = 0,
			.argInfo    = POPT_ARG_STRING,
			.arg        = &opt_domain,
			.val        = OPT_SC_CHANGE_PWD,
			.descrip    = "Change a secure channel password for domain on server",
			.argDescrip = "DOMAIN",
		},
		{
			.longName   = "dsgetdc",
			.shortName  = 0,
			.argInfo    = POPT_ARG_STRING,
			.arg        = &opt_domain,
			.val        = OPT_DSGETDC,
			.descrip    = "Call DsGetDcName",
			.argDescrip = "DOMAIN",
		},
		{
			.longName   = "pdc",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_pdc,
			.val        = OPT_PDC,
			.descrip    = NULL,
		},
		{
			.longName   = "ds",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_ds,
			.val        = OPT_DS,
			.descrip    = NULL,
		},
		{
			.longName   = "dsp",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_dsp,
			.val        = OPT_DSP,
			.descrip    = NULL,
		},
		{
			.longName   = "gc",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_gc,
			.val        = OPT_GC,
			.descrip    = NULL,
		},
		{
			.longName   = "kdc",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_kdc,
			.val        = OPT_KDC,
			.descrip    = NULL,
		},
		{
			.longName   = "timeserv",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_timeserv,
			.val        = OPT_TIMESERV,
			.descrip    = NULL,
		},
		{
			.longName   = "gtimeserv",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_gtimeserv,
			.val        = OPT_GTIMESERV,
			.descrip    = NULL,
		},
		{
			.longName   = "ws",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_ws,
			.val        = OPT_WS,
			.descrip    = NULL,
		},
		{
			.longName   = "netbios",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_netbios,
			.val        = OPT_NETBIOS,
			.descrip    = NULL,
		},
		{
			.longName   = "dns",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_dns,
			.val        = OPT_DNS,
			.descrip    = NULL,
		},
		{
			.longName   = "ip",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_ip,
			.val        = OPT_IP,
			.descrip    = NULL,
		},
		{
			.longName   = "force",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_force,
			.val        = OPT_FORCE,
			.descrip    = NULL,
		},
		{
			.longName   = "writable",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_writable,
			.val        = OPT_WRITABLE,
			.descrip    = NULL,
		},
		{
			.longName   = "avoidself",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_avoidself,
			.val        = OPT_AVOIDSELF,
			.descrip    = NULL,
		},
		{
			.longName   = "ldaponly",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_ldaponly,
			.val        = OPT_LDAPONLY,
			.descrip    = NULL,
		},
		{
			.longName   = "backg",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_backg,
			.val        = OPT_BACKG,
			.descrip    = NULL,
		},
		{
			.longName   = "ds_6",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_ds_6,
			.val        = OPT_DS_6,
			.descrip    = NULL,
		},
		{
			.longName   = "try_next_closest_site",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_try_next_closest_site,
			.val        = OPT_TRY_NEXT_CLOSEST_SITE,
			.descrip    = NULL,
		},
		{
			.longName   = "site",
			.shortName  = 0,
			.argInfo    = POPT_ARG_STRING,
			.arg        = &opt_site,
			.val        = OPT_SITE,
			.descrip    = "SITE",
		},
		{
			.longName   = "account",
			.shortName  = 0,
			.argInfo    = POPT_ARG_STRING,
			.arg        = &opt_account,
			.val        = OPT_ACCOUNT,
			.descrip    = "ACCOUNT",
		},
		{
			.longName   = "ret_dns",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_ret_dns,
			.val        = OPT_RET_DNS,
			.descrip    = NULL,
		},
		{
			.longName   = "ret_netbios",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_ret_netbios,
			.val        = OPT_RET_NETBIOS,
			.descrip    = NULL,
		},
		{
			.longName   = "dsregdns",
			.shortName  = 0,
			.argInfo    = POPT_ARG_NONE,
			.arg        = &opt_dsregdns,
			.val        = OPT_DSREGDNS,
			.descrip    = "Force registration of all DC-specific DNS records",
		},
		POPT_COMMON_LIBNETAPI_EXAMPLES
		POPT_TABLEEND
	};

	status = libnetapi_init(&ctx);
	if (status != 0) {
		return status;
	}

	pc = poptGetContext("nltest", argc, argv, long_options, 0);

	poptSetOtherOptionHelp(pc, "<options>");
	while((opt = poptGetNextOpt(pc)) != -1) {
	}

	if (argc == 1) {
		poptPrintHelp(pc, stderr, 0);
		goto done;
	}

	poptResetContext(pc);

	while ((opt = poptGetNextOpt(pc)) != -1) {
		switch (opt) {

		case OPT_SERVER:

			if ((opt_server[0] == '/' && opt_server[1] == '/') ||
			    (opt_server[0] == '\\' && opt_server[1] ==  '\\')) {
				opt_server += 2;
			}

			break;

		case OPT_DBFLAG:
			query_level = 1;
			status = I_NetLogonControl2(opt_server,
						    NETLOGON_CONTROL_SET_DBFLAG,
						    query_level,
						    (uint8_t *)&opt_dbflag,
						    &buffer);
			if (status != 0) {
				fprintf(stderr, "I_NetlogonControl failed: Status = %d 0x%x %s\n",
					status, status,
					libnetapi_get_error_string(ctx, status));
				goto done;
			}

			print_netlogon_info_result(query_level, buffer);

			break;
		case OPT_SC_QUERY:
			query_level = 2;
			status = I_NetLogonControl2(opt_server,
						    NETLOGON_CONTROL_TC_QUERY,
						    query_level,
						    (uint8_t *)opt_domain,
						    &buffer);
			if (status != 0) {
				fprintf(stderr, "I_NetlogonControl failed: Status = %d 0x%x %s\n",
					status, status,
					libnetapi_get_error_string(ctx, status));
				goto done;
			}

			print_netlogon_info_result(query_level, buffer);

			break;
		case OPT_SC_VERIFY:
			query_level = 2;
			status = I_NetLogonControl2(opt_server,
						    NETLOGON_CONTROL_TC_VERIFY,
						    query_level,
						    (uint8_t *)opt_domain,
						    &buffer);
			if (status != 0) {
				fprintf(stderr, "I_NetlogonControl failed: Status = %d 0x%x %s\n",
					status, status,
					libnetapi_get_error_string(ctx, status));
				goto done;
			}

			print_netlogon_info_result(query_level, buffer);

			break;
		case OPT_SC_RESET:
			query_level = 2;
			status = I_NetLogonControl2(opt_server,
						    NETLOGON_CONTROL_REDISCOVER,
						    query_level,
						    (uint8_t *)opt_domain,
						    &buffer);
			if (status != 0) {
				fprintf(stderr, "I_NetlogonControl failed: Status = %d 0x%x %s\n",
					status, status,
					libnetapi_get_error_string(ctx, status));
				goto done;
			}

			print_netlogon_info_result(query_level, buffer);

			break;
		case OPT_SC_CHANGE_PWD:
			query_level = 1;
			status = I_NetLogonControl2(opt_server,
						    NETLOGON_CONTROL_CHANGE_PASSWORD,
						    query_level,
						    (uint8_t *)opt_domain,
						    &buffer);
			if (status != 0) {
				fprintf(stderr, "I_NetlogonControl failed: Status = %d 0x%x %s\n",
					status, status,
					libnetapi_get_error_string(ctx, status));
				goto done;
			}

			print_netlogon_info_result(query_level, buffer);

			break;
		case OPT_DSREGDNS:
			query_level = 1;
			status = I_NetLogonControl2(opt_server,
						    NETLOGON_CONTROL_FORCE_DNS_REG,
						    query_level,
						    NULL,
						    &buffer);
			if (status != 0) {
				fprintf(stderr, "I_NetlogonControl failed: Status = %d 0x%x %s\n",
					status, status,
					libnetapi_get_error_string(ctx, status));
				goto done;
			}

			print_netlogon_info_result(query_level, buffer);

			break;
		case OPT_DSGETDC:
			if (opt_pdc)
				flags |= DS_PDC_REQUIRED;
			if (opt_ds)
				flags |= DS_DIRECTORY_SERVICE_REQUIRED;
			if (opt_dsp)
				flags |= DS_DIRECTORY_SERVICE_PREFERRED;
			if (opt_kdc)
				flags |= DS_KDC_REQUIRED;
			if (opt_timeserv)
				flags |= DS_TIMESERV_REQUIRED;
			if (opt_gtimeserv)
				flags |= DS_GOOD_TIMESERV_PREFERRED;
			if (opt_ws)
				flags |= DS_WEB_SERVICE_REQUIRED;
			if (opt_netbios)
				flags |= DS_IS_FLAT_NAME;
			if (opt_dns)
				flags |= DS_IS_DNS_NAME;
			if (opt_ip)
				flags |= DS_IP_REQUIRED;
			if (opt_force)
				flags |= DS_FORCE_REDISCOVERY;
			if (opt_writable)
				flags |= DS_WRITABLE_REQUIRED;
			if (opt_avoidself)
				flags |= DS_AVOID_SELF;
			if (opt_ldaponly)
				flags |= DS_ONLY_LDAP_NEEDED;
			if (opt_backg)
				flags |= DS_BACKGROUND_ONLY;
			if (opt_ds_6)
				flags |= DS_DIRECTORY_SERVICE_6_REQUIRED;
			if (opt_try_next_closest_site)
				flags |= DS_TRY_NEXTCLOSEST_SITE;
			if (opt_ret_dns)
				flags |= DS_RETURN_DNS_NAME;
			if (opt_ret_netbios)
				flags |= DS_RETURN_FLAT_NAME;

			status = DsGetDcName(opt_server,
					     opt_domain,
					     NULL, /* domain_guid */
					     opt_site,
					     flags,
					     &dc_info);
			if (status != 0) {
				fprintf(stderr, "DsGetDcName failed: Status = %d 0x%x %s\n",
					status, status,
					libnetapi_get_error_string(ctx, status));
				goto done;
			}

			print_dc_info(dc_info);

			break;
		default:
			continue;
		}
	}

	printf("The command completed successfully\n");
	status = 0;

 done:

	printf("\n");
	libnetapi_free(ctx);
	poptFreeContext(pc);

	return status;
}
