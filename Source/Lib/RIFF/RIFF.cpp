/*  Copyright (c) MediaArea.net SARL & AV Preservation by reto.ch.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Lib/RIFF/RIFF.h"
#include "Lib/RAWcooked/RAWcooked.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Tested cases
enum endianess
{
    BE, // Or Signed for 8-bit
    LE, // Or Unsigned for 8-bit
};

struct riff_tested
{
    uint32_t                    SamplesPerSec;
    uint8_t                     BitDepth;
    uint8_t                     Channels;
    endianess                   Endianess;
    riff::style                 Style;
};

const size_t RIFF_Tested_Size = 27;
struct riff_tested RIFF_Tested[RIFF_Tested_Size] =
{
    { 44100,  8, 1, LE, riff::PCM_44100_8_1_U },
    { 44100,  8, 2, LE, riff::PCM_44100_8_2_U },
    { 44100,  8, 6, LE, riff::PCM_44100_8_6_U },
    { 44100, 16, 1, LE, riff::PCM_44100_16_1_LE },
    { 44100, 16, 2, LE, riff::PCM_44100_16_2_LE },
    { 44100, 16, 6, LE, riff::PCM_44100_16_6_LE },
    { 44100, 24, 1, LE, riff::PCM_44100_24_1_LE },
    { 44100, 24, 2, LE, riff::PCM_44100_24_2_LE },
    { 44100, 24, 6, LE, riff::PCM_44100_24_6_LE },
    { 48000,  8, 1, LE, riff::PCM_44100_8_1_U },
    { 48000,  8, 2, LE, riff::PCM_44100_8_2_U },
    { 48000,  8, 6, LE, riff::PCM_44100_8_6_U },
    { 48000, 16, 1, LE, riff::PCM_48000_16_1_LE },
    { 48000, 16, 2, LE, riff::PCM_48000_16_2_LE },
    { 48000, 16, 6, LE, riff::PCM_48000_16_6_LE },
    { 48000, 24, 1, LE, riff::PCM_48000_24_1_LE },
    { 48000, 24, 2, LE, riff::PCM_48000_24_2_LE },
    { 48000, 24, 6, LE, riff::PCM_48000_24_6_LE },
    { 96000,  8, 1, LE, riff::PCM_44100_8_1_U },
    { 96000,  8, 2, LE, riff::PCM_44100_8_2_U },
    { 96000,  8, 6, LE, riff::PCM_44100_8_6_U },
    { 96000, 16, 1, LE, riff::PCM_96000_16_1_LE },
    { 96000, 16, 2, LE, riff::PCM_96000_16_2_LE },
    { 96000, 16, 6, LE, riff::PCM_96000_16_6_LE },
    { 96000, 24, 1, LE, riff::PCM_96000_24_1_LE },
    { 96000, 24, 2, LE, riff::PCM_96000_24_2_LE },
    { 96000, 24, 6, LE, riff::PCM_96000_24_6_LE },
};

//---------------------------------------------------------------------------


#define ELEMENT_BEGIN(_VALUE) \
riff::call riff::SubElements_##_VALUE(uint64_t Name) \
{ \
    switch (Name) \
    { \

#define ELEMENT_CASE(_VALUE,_NAME) \
    case 0x##_VALUE:  Levels[Level].SubElements = &riff::SubElements_##_NAME;  return &riff::_NAME;

#define ELEMENT_VOID(_VALUE,_NAME) \
    case 0x##_VALUE:  Levels[Level].SubElements = &riff::SubElements_Void;  return &riff::_NAME;


#define ELEMENT_END() \
    default:                        return SubElements_Void(Name); \
    } \
} \

ELEMENT_BEGIN(_)
ELEMENT_CASE(57415645, WAVE)
ELEMENT_END()

ELEMENT_BEGIN(WAVE)
ELEMENT_VOID(64617461, WAVE_data)
ELEMENT_VOID(666D7420, WAVE_fmt_)
ELEMENT_END()


//---------------------------------------------------------------------------
riff::call riff::SubElements_Void(uint64_t Name)
{
    Levels[Level].SubElements = &riff::SubElements_Void; return &riff::Void;
}

//***************************************************************************
// Errors
//***************************************************************************

//---------------------------------------------------------------------------
const char* riff::ErrorMessage()
{
    return error_message;
}

//***************************************************************************
// RIFF
//***************************************************************************

//---------------------------------------------------------------------------
riff::riff() :
    RAWcooked(NULL),
    IsDetected(false),
    Style(PCM_Style_Max),
    error_message(NULL)
{
}

//---------------------------------------------------------------------------
uint16_t riff::Get_L2()
{
    uint16_t ToReturn = Buffer[Buffer_Offset + 0] | (Buffer[Buffer_Offset + 1] << 8);
    Buffer_Offset += 2;
    return ToReturn;
}

//---------------------------------------------------------------------------
uint32_t riff::Get_L4()
{
    uint32_t ToReturn = Buffer[Buffer_Offset+0] | (Buffer[Buffer_Offset + 1] << 8) | (Buffer[Buffer_Offset + 2] << 16) | (Buffer[Buffer_Offset + 3] << 24);
    Buffer_Offset += 4;
    return ToReturn;
}

//---------------------------------------------------------------------------
uint32_t riff::Get_B4()
{
    uint32_t ToReturn = (Buffer[Buffer_Offset + 0] << 24) | (Buffer[Buffer_Offset + 1] << 16) | (Buffer[Buffer_Offset + 2] << 8) | Buffer[Buffer_Offset + 3];
    Buffer_Offset += 4;
    return ToReturn;
}

//---------------------------------------------------------------------------
bool riff::Parse(bool AcceptTruncated)
{
    if (Buffer_Size < 12)
        return false;
    if (Buffer[8] != 'W' || Buffer[9] != 'A' || Buffer[10] != 'V' || Buffer[11] != 'E')
        return false;
    if (Buffer[0] == 'R' && Buffer[1] == 'F' && Buffer[2] == '6' && Buffer[3] == '4')
    {
        IsDetected = true;
        Error("RF64 (4GB+ WAV)");
        return false;
    }
    if (Buffer[0] != 'R' || Buffer[1] != 'I' || Buffer[2] != 'F' || Buffer[3] != 'F')
        return false;
    IsDetected = true;

    Buffer_Offset = 0;
    Levels[0].Offset_End = Buffer_Size;
    Levels[0].SubElements = &riff::SubElements__;
    Level=1;

    while (Buffer_Offset < Buffer_Size)
    {
        // Find the current nesting level
        while (Buffer_Offset >= Levels[Level - 1].Offset_End)
        {
            Levels[Level].SubElements = NULL;
            Level--;
        }

        // Parse the chunk header
        uint32_t Name;
        uint64_t Size;
        if (Buffer_Offset + 8 <= Levels[Level - 1].Offset_End)
        {
            Name = Get_B4();
            Size = Get_L4();
            if (Name == 0x52494646) // "RIFF"
            {
                if (Size < 4)
                {
                    Error("Incoherency detected while parsing RIFF");
                    return false;
                }
                Name = Get_B4();
                Size -= 4;
            }
            if (Buffer_Offset + Size > Levels[Level - 1].Offset_End)
            {
                // Truncated
                if (!AcceptTruncated)
                {
                    Error("Truncated RIFF?");
                    return false;
                }
                Size = Levels[Level - 1].Offset_End - Buffer_Offset;
            }
        }
        else
        {
            // There is a problem in the chunk
            Error("Incoherency detected while parsing RIFF");
            return false;
        }

        // Parse the chunk content
        Levels[Level].Offset_End = Buffer_Offset + Size;
        call Call = (this->*Levels[Level - 1].SubElements)(Name);
        IsList = false;
        (this->*Call)();
        if (!IsList)
        {
            Buffer_Offset = Levels[Level].Offset_End;

            // Padding byte
            if (Buffer_Offset % 2 && Buffer_Offset < Buffer_Size && !Buffer[Buffer_Offset])
            {
                Buffer_Offset++;
                Levels[Level].Offset_End = Buffer_Offset;
            }
        }

        // Next chunk (or sub-chunk)
        if (Buffer_Offset < Levels[Level].Offset_End)
            Level++;
    }

    return 0;
}

//---------------------------------------------------------------------------
uint8_t riff::BitDepth()
{
    return RIFF_Tested[Style].BitDepth;
}

//---------------------------------------------------------------------------
uint8_t riff::Endianess()
{
    return RIFF_Tested[Style].Endianess;
}

//---------------------------------------------------------------------------
void riff::Void()
{
}

//---------------------------------------------------------------------------
void riff::WAVE()
{
    IsList = true;
}

//---------------------------------------------------------------------------
void riff::WAVE_data()
{
    // Write RAWcooked file
    if (RAWcooked)
    {
        RAWcooked->Unique = true;
        RAWcooked->Before = Buffer;
        RAWcooked->Before_Size = Buffer_Offset;
        RAWcooked->After = Buffer + Levels[Level].Offset_End;
        RAWcooked->After_Size = Buffer_Size - Levels[Level].Offset_End;
        RAWcooked->Parse();
    }
}

//---------------------------------------------------------------------------
void riff::WAVE_fmt_()
{
    if (Levels[Level].Offset_End - Buffer_Offset < 16)
    {
        Error("WAV FormatTag format");
        return;
    }

    endianess Endianess;
    uint16_t FormatTag = Get_L2();
    if (FormatTag == 1 || FormatTag == 0xFFFE)
        Endianess = LE;
    else
    {
        Error("WAV FormatTag is not WAVE_FORMAT_PCM 1");
        return;
    }

    uint16_t Channels = Get_L2();
    uint32_t SamplesPerSec = Get_L4();
    uint32_t AvgBytesPerSec = Get_L4();
    uint16_t BlockAlign = Get_L2();
    uint16_t BitDepth = Get_L2();

    if (AvgBytesPerSec * 8 != Channels * BitDepth * SamplesPerSec)
    {
        Error("WAV BlockAlign not supported");
        return;
    }
    if (BlockAlign * 8 != Channels * BitDepth)
    {
        Error("WAV BlockAlign not supported");
        return;
    }
    if (FormatTag == 1)
    {
        if (Levels[Level].Offset_End == Buffer_Offset + 2)
        {
            uint16_t Padding0 = Get_L2(); // Some files have 2 zeroes, it does not hurt so we accept them
            if (Padding0)
            {
                Error("WAV FormatTag extension");
                return;
            }
        }

        if (Levels[Level].Offset_End == Buffer_Offset + 4)
        {
            uint32_t Padding0 = Get_L4(); // Some files have 4 zeroes, it does not hurt so we accept them
            if (Padding0)
            {
                Error("WAV FormatTag extension");
                return;
            }
        }

        if (Levels[Level].Offset_End - Buffer_Offset)
        {
            Error("WAV FormatTag extension");
            return;
        }
    }
    if (FormatTag == 0xFFFE)
    {
        if (Levels[Level].Offset_End - Buffer_Offset != 24)
        {
            Error("WAV FormatTag extension");
            return;
        }
        uint16_t cbSize = Get_L2();
        if (cbSize != 22)
        {
            Error("WAV FormatTag cbSize");
            return;
        }
        uint16_t ValidBitsPerSample = Get_L2();
        if (ValidBitsPerSample != BitDepth)
        {
            Error("WAV FormatTag ValidBitsPerSample");
            return;
        }
        uint32_t ChannelMask = Get_L4();
        if ((Channels != 1 || ChannelMask != 0x04)
         && (Channels != 2 || ChannelMask != 0x03)
         && (Channels != 6 || (ChannelMask != 0x3F && ChannelMask != 0x60F)))
        {
            Error("WAV FormatTag ChannelMask");
            return;
        }
        uint32_t SubFormat1 = Get_L4();
        uint32_t SubFormat2 = Get_L4();
        uint32_t SubFormat3 = Get_B4();
        uint64_t SubFormat4 = Get_B4();
        if (SubFormat1 != 0x00000001
         || SubFormat2 != 0x00100000
         || SubFormat3 != 0x800000aa
         || SubFormat4 != 0x00389b71)
        {
            Error("WAV SubFormat is not KSDATAFORMAT_SUBTYPE_PCM 00000001-0000-0010-8000-00AA00389B71");
            return;
        }
    }

    // Supported?
    size_t Tested = 0;
    for (; Tested < RIFF_Tested_Size; Tested++)
    {
        riff_tested& RIFF_Tested_Item = RIFF_Tested[Tested];
        if (RIFF_Tested_Item.SamplesPerSec == SamplesPerSec
            && RIFF_Tested_Item.BitDepth == BitDepth
            && RIFF_Tested_Item.Channels == Channels
            && RIFF_Tested_Item.Endianess == Endianess)
            break;
    }
    if (Tested >= RIFF_Tested_Size)
    {
        Error("Style (SamplesPerSec / BitDepth / Channels / Endianess combination)");
        return;
    }
    Style = RIFF_Tested[Tested].Style;
}

