/*
** Copyright (C) 2002-2013 Sourcefire, Inc.
** Copyright (C) 1998-2002 Martin Roesch <roesch@sourcefire.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
// cd_gtp.cc author Josh Rosenbaum <jorosenba@cisco.com>



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "snort_debug.h"
#include "main/snort.h"
#include "framework/codec.h"
#include "protocols/packet.h"
#include "codecs/codec_events.h"
#include "codecs/misc/cd_gtp_module.h"
#include "protocols/ipv4.h"
#include "protocols/ipv6.h"
#include "packet_io/active.h"
#include "codecs/sf_protocols.h"
#include "protocols/protocol_ids.h"

namespace
{

class GtpCodec : public Codec
{
public:
    GtpCodec() : Codec(CD_GTP_NAME){};
    ~GtpCodec(){};

    virtual PROTO_ID get_proto_id() { return PROTO_GTP; };
    virtual void get_protocol_ids(std::vector<uint16_t>& v);
    virtual bool decode(const uint8_t *raw_pkt, const uint32_t len, 
        Packet *, uint16_t &lyr_len, uint16_t &next_prot_id);
    virtual bool encode(EncState*, Buffer* out, const uint8_t* raw_in);
    virtual bool update(Packet*, Layer*, uint32_t* len);
};


/* GTP basic Header  */
struct GTPHdr
{
    uint8_t  flag;              /* flag: version (bit 6-8), PT (5), E (3), S (2), PN (1) */
    uint8_t  type;              /* message type */
    uint16_t length;            /* length */
};


} // anonymous namespace


static const uint32_t GTP_MIN_LEN = 8;
static const uint32_t GTP_V0_HEADER_LEN = 20;
static const uint32_t GTP_V1_HEADER_LEN = 12;


void GtpCodec::get_protocol_ids(std::vector<uint16_t>& v)
{
    v.push_back(PROTOCOL_GTP);
}

/* Function: DecodeGTP(uint8_t *, uint32_t, Packet *)
 *
 * GTP (GPRS Tunneling Protocol) is layered over UDP.
 * Decode these (if present) and go to DecodeIPv6/DecodeIP.
 *
 */

bool GtpCodec::decode(const uint8_t *raw_pkt, const uint32_t len, 
    Packet *p, uint16_t &lyr_len, uint16_t &next_prot_id)
{
    uint8_t  next_hdr_type;
    uint8_t  version;
    uint8_t  ip_ver;
    GTPHdr *hdr;

    DEBUG_WRAP(DebugMessage(DEBUG_DECODE, "Start GTP decoding.\n"););

    hdr = (GTPHdr *) raw_pkt;

    if (p->GTPencapsulated)
    {
        codec_events::decoder_alert_encapsulated(p, DECODE_GTP_MULTIPLE_ENCAPSULATION,
                raw_pkt, len);
        return false;
    }
    else
    {
        p->GTPencapsulated = 1;
    }
    /*Check the length*/
    if (len < GTP_MIN_LEN)
       return false;
    /* We only care about PDU*/
    if ( hdr->type != 255)
       return false;
    /*Check whether this is GTP or GTP', Exit if GTP'*/
    if (!(hdr->flag & 0x10))
       return false;

    /*The first 3 bits are version number*/
    version = (hdr->flag & 0xE0) >> 5;
    switch (version)
    {
    case 0: /*GTP v0*/
        DEBUG_WRAP(DebugMessage(DEBUG_DECODE, "GTP v0 packets.\n"););

        lyr_len = GTP_V0_HEADER_LEN;
        /*Check header fields*/
        if (len < lyr_len)
        {
            codec_events::decoder_event(p, DECODE_GTP_BAD_LEN);
            return false;
        }

        p->proto_bits |= PROTO_BIT__GTP;

        /*Check the length field. */
        if (len != ((unsigned int)ntohs(hdr->length) + lyr_len))
        {
            DEBUG_WRAP(DebugMessage(DEBUG_DECODE, "Calculated length %d != %d in header.\n",
                    len - lyr_len, ntohs(hdr->length)););
            codec_events::decoder_event(p, DECODE_GTP_BAD_LEN);
            return false;
        }

        break;
    case 1: /*GTP v1*/
        DEBUG_WRAP(DebugMessage(DEBUG_DECODE, "GTP v1 packets.\n"););

        /*Check the length based on optional fields and extension header*/
        if (hdr->flag & 0x07)
        {

            lyr_len =  GTP_V1_HEADER_LEN;

            /*Check optional fields*/
            if (len < lyr_len)
            {
                codec_events::decoder_event(p, DECODE_GTP_BAD_LEN);
                return false;
            }
            next_hdr_type = *(raw_pkt + lyr_len - 1);

            /*Check extension headers*/
            while (next_hdr_type)
            {
                uint16_t ext_hdr_len;
                /*check length before reading data*/
                if (len < (uint32_t)(lyr_len + 4))
                {
                    codec_events::decoder_event(p, DECODE_GTP_BAD_LEN);
                    return false;
                }

                ext_hdr_len = *(raw_pkt + lyr_len);

                if (!ext_hdr_len)
                {
                    codec_events::decoder_event(p, DECODE_GTP_BAD_LEN);
                    return false;
                }
                /*Extension header length is a unit of 4 octets*/
                lyr_len += ext_hdr_len * 4;

                /*check length before reading data*/
                if (len < lyr_len)
                {
                    codec_events::decoder_event(p, DECODE_GTP_BAD_LEN);
                    return false;
                }
                next_hdr_type = *(raw_pkt + lyr_len - 1);
            }
        }
        else
            lyr_len = GTP_MIN_LEN;

        p->proto_bits |= PROTO_BIT__GTP;

        /*Check the length field. */
        if (len != ((unsigned int)ntohs(hdr->length) + GTP_MIN_LEN))
        {
            DEBUG_WRAP(DebugMessage(DEBUG_DECODE, "Calculated length %d != %d in header.\n",
                    len - GTP_MIN_LEN, ntohs(hdr->length)););
            codec_events::decoder_event(p, DECODE_GTP_BAD_LEN);
            return false;
        }

        break;
    default:
        DEBUG_WRAP(DebugMessage(DEBUG_DECODE, "Unknown protocol version.\n"););
        return false;

    }

    if ( ScTunnelBypassEnabled(TUNNEL_GTP) )
        Active_SetTunnelBypass();


    if (len > 0)
    {
        p->packet_flags |= PKT_UNSURE_ENCAP;

        ip_ver = *(raw_pkt + GTP_MIN_LEN) & 0xF0;
        if (ip_ver == 0x40)
            next_prot_id = ipv4::prot_id(); 
        else if (ip_ver == 0x60)
            next_prot_id = ipv6::prot_id();
    }
    
    return true;
}



/******************************************************************
 ******************** E N C O D E R  ******************************
 ******************************************************************/
 
static inline bool update_GTP_length(GTPHdr* h, int gtp_total_len )
{
    /*The first 3 bits are version number*/
    uint8_t version = (h->flag & 0xE0) >> 5;
    switch (version)
    {
    case 0: /*GTP v0*/
        h->length = htons((uint16_t)(gtp_total_len - GTP_V0_HEADER_LEN));
        break;
    case 1: /*GTP v1*/
        h->length = htons((uint16_t)(gtp_total_len - GTP_MIN_LEN));
        break;
    default:
        return false;
    }
    return false;

}

bool GtpCodec::encode (EncState* enc, Buffer* out, const uint8_t* raw_in)
{
    int n = enc->p->layers[enc->layer-1].length;

    if (!update_buffer(out, n))
        return false;

    const GTPHdr *hi = reinterpret_cast<const GTPHdr*>(raw_in);
    GTPHdr *ho = reinterpret_cast<GTPHdr*>(out->base);
    memcpy(ho, hi, n);

    return update_GTP_length(ho, out->end);
}

bool GtpCodec::update (Packet*, Layer* lyr, uint32_t* len)
{
    GTPHdr* h = (GTPHdr*)(lyr->start);
    *len += lyr->length;
    return( update_GTP_length(h,*len));
}

//-------------------------------------------------------------------------
// api
//-------------------------------------------------------------------------

static Module* mod_ctor()
{
    return new GtpModule;
}

static void mod_dtor(Module* m)
{
    delete m;
}

static Codec* ctor(Module*)
{
    return new GtpCodec();
}

static void dtor(Codec *cd)
{
    delete cd;
}

static const CodecApi gtp_api =
{
    {
        PT_CODEC,
        CD_GTP_NAME,
        CDAPI_PLUGIN_V0,
        0,
        mod_ctor,
        mod_dtor
    },
    nullptr, // pinit
    nullptr, // pterm
    nullptr, // tinit
    nullptr, // tterm
    ctor, // ctor
    dtor, // dtor
};

#ifdef BUILDING_SO
SO_PUBLIC const BaseApi* snort_plugins[] =
{
    &gtp_api.base,
    nullptr
};
#else
const BaseApi* cd_gtp = &gtp_api.base;
#endif
