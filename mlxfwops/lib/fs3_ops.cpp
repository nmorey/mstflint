/*
 * Copyright (C) Jan 2013 Mellanox Technologies Ltd. All rights reserved.
 * 
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 * 
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 * 
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 * 
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <stdlib.h>

#include "fs3_ops.h"

#include <vector>


const u_int32_t Fs3Operations::_itocSignature[4] = {
        ITOC_ASCII,   // Ascii of "MTFW"
        TOC_RAND1,   // Random data
        TOC_RAND2,
        TOC_RAND3
};

const Fs3Operations::SectionInfo Fs3Operations::_fs3SectionsInfoArr[] = {
    {FS3_END,           "END"},
    {FS3_ITOC,          "ITOC_Header"},

    {FS3_BOOT_CODE,     "BOOT_CODE"},
    {FS3_PCI_CODE,      "PCI_CODE"},
    {FS3_MAIN_CODE,     "MAIN_CODE"},
    {FS3_PCIE_LINK_CODE, "PCIE_LINK_CODE"},
    {FS3_IRON_PREP_CODE, "IRON_PREP_CODE"},
    {FS3_POST_IRON_BOOT_CODE, "POST_IRON_BOOT_CODE"},
    {FS3_HW_BOOT_CFG,   "HW_BOOT_CFG"},
    {FS3_HW_MAIN_CFG,   "HW_MAIN_CFG"},
    {FS3_IMAGE_INFO,    "IMAGE_INFO"},
    {FS3_FW_BOOT_CFG,   "FW_BOOT_CFG"},
    {FS3_FW_MAIN_CFG,   "FW_MAIN_CFG"},
    {FS3_ROM_CODE,      "ROM_CODE"},
    {FS3_DBG_LOG_MAP,   "DBG_LOG_MAP"},
    {FS3_DBG_FW_INI,    "DBG_FW_INI"},
    {FS3_DBG_FW_PARAMS, "DBG_FW_PARAMS"},
    {FS3_FW_ADB,        "FW_ADB"},

    {FS3_MFG_INFO,      MFG_INFO},
    {FS3_DEV_INFO,      "DEV_INFO"},
    {FS3_NV_DATA,       "NV_DATA"},
    {FS3_VPD_R0,        "VPD_R0"},
};

bool Fs3Operations::Fs3UpdateImgCache(u_int8_t *buff, u_int32_t addr, u_int32_t size)
{
    // vector<u_int8_t>::iterator it;
    u_int32_t min_required_size = addr + size;
    u_int32_t i;

    if (_fs3ImgInfo.imageCache.size() < min_required_size) {
        _fs3ImgInfo.imageCache.resize(min_required_size);
    }
    for (i = 0; i < size; i++) {
        _fs3ImgInfo.imageCache.at(addr + i) = buff[i];
    }
    return true;
}

bool Fs3Operations::UpdateImgCache(u_int8_t *buff, u_int32_t addr, u_int32_t size)
{
    return Fs3UpdateImgCache(buff, addr, size);
}

const char *Fs3Operations::GetSectionNameByType(u_int8_t section_type) {

    for (u_int32_t i = 0; i < ARR_SIZE(_fs3SectionsInfoArr); i++) {
        const SectionInfo *sect_info = &_fs3SectionsInfoArr[i];
        if (sect_info->type == section_type) {
            return sect_info->name;
        }
    }
    return UNKNOWN_SECTION;
}

bool Fs3Operations::DumpFs3CRCCheck(u_int8_t sect_type, u_int32_t sect_addr, u_int32_t sect_size, u_int32_t crc_act,
        u_int32_t crc_exp, bool ignore_crc, VerifyCallBack verifyCallBackFunc)
{
    char pr[256];
    const char *sect_type_str = GetSectionNameByType(sect_type);
    sprintf(pr, CRC_CHECK_OLD, PRE_CRC_OUTPUT, sect_addr, sect_addr + sect_size - 1, sect_size,
            sect_type_str);
    if (!strcmp(sect_type_str, UNKNOWN_SECTION)) {
        sprintf(pr + strlen(pr), ":0x%x", sect_type);
    }
    sprintf(pr + strlen(pr), ")");
    return  CheckAndPrintCrcRes(pr, 0, sect_addr, crc_exp, crc_act, ignore_crc, verifyCallBackFunc);

}

bool Fs3Operations::CheckTocSignature(struct cibfw_itoc_header *itoc_header, u_int32_t first_signature)
{
    if ( itoc_header->signature0 != first_signature  ||
         itoc_header->signature1 !=  TOC_RAND1       ||
         itoc_header->signature2 !=  TOC_RAND2       ||
         itoc_header->signature3 !=  TOC_RAND3) {
        return false;
    }
    return true;
}

#define CHECK_UID_STRUCTS_SIZE(uids_context, cibfw_guids_context) {\
	    if (sizeof(uids_context) != sizeof(cibfw_guids_context)) {\
	        return errmsg("Internal error: Size of uids_t (%d) is not equal to size of  struct cibfw_guids guids (%d)\n",\
	                (int)sizeof(uids_context), (int)sizeof(cibfw_guids_context));\
	    }\
}
bool Fs3Operations::GetMfgInfo(u_int8_t *buff)
{
    struct cibfw_mfg_info mfg_info;
    cibfw_mfg_info_unpack(&mfg_info, buff);
    // cibfw_mfg_info_dump(&mfg_info, stdout);

    CHECK_UID_STRUCTS_SIZE(_fs3ImgInfo.ext_info.orig_fs3_uids_info, mfg_info.guids);
    memcpy(&_fs3ImgInfo.ext_info.orig_fs3_uids_info, &mfg_info.guids, sizeof(mfg_info.guids));
    strcpy(_fs3ImgInfo.ext_info.orig_psid, mfg_info.psid);

    _fs3ImgInfo.ext_info.guids_override_en = mfg_info.guids_override_en;
    return true;

}

bool Fs3Operations::GetImageInfo(u_int8_t *buff)
{

    struct cibfw_image_info image_info;
    cibfw_image_info_unpack(&image_info, buff);
    // cibfw_image_info_dump(&image_info, stdout);

    _fwImgInfo.ext_info.fw_ver[0] = image_info.FW_VERSION.MAJOR;
    _fwImgInfo.ext_info.fw_ver[1] = image_info.FW_VERSION.MINOR;
    _fwImgInfo.ext_info.fw_ver[2] = image_info.FW_VERSION.SUBMINOR;

    _fwImgInfo.ext_info.mic_ver[0] = image_info.mic_version.MAJOR;
    _fwImgInfo.ext_info.mic_ver[1] = image_info.mic_version.MINOR;
    _fwImgInfo.ext_info.mic_ver[2] = image_info.mic_version.SUBMINOR;


    strcpy(_fs3ImgInfo.ext_info.image_vsd, image_info.vsd);
    strcpy(_fwImgInfo.ext_info.psid, image_info.psid);
    strcpy(_fwImgInfo.ext_info.product_ver, image_info.prod_ver);
    return true;
}

bool Fs3Operations::GetDevInfo(u_int8_t *buff)
{
    struct cibfw_device_info dev_info;
    cibfw_device_info_unpack(&dev_info, buff);
    // cibfw_device_info_dump(&dev_info, stdout);

    CHECK_UID_STRUCTS_SIZE(_fs3ImgInfo.ext_info.fs3_uids_info, dev_info.guids);
    memcpy(&_fs3ImgInfo.ext_info.fs3_uids_info, &dev_info.guids, sizeof(dev_info.guids));
    strcpy(_fwImgInfo.ext_info.vsd, dev_info.vsd);
    _fwImgInfo.ext_info.vsd_sect_found = true;
    return true;
}




bool Fs3Operations::GetImageInfoFromSection(u_int8_t *buff, u_int8_t sect_type, u_int8_t check_support_only)
{
    #define EXEC_GET_INFO_OR_GET_SUPPORT(get_info_func, buff, check_support_only) (check_support_only) ? true : get_info_func(buff);

    switch (sect_type) {
        case FS3_MFG_INFO:
            return EXEC_GET_INFO_OR_GET_SUPPORT(GetMfgInfo, buff, check_support_only);
        case FS3_IMAGE_INFO:
            return EXEC_GET_INFO_OR_GET_SUPPORT(GetImageInfo, buff, check_support_only);
        case FS3_DEV_INFO:
            return EXEC_GET_INFO_OR_GET_SUPPORT(GetDevInfo, buff, check_support_only);
    }

    if (check_support_only) {
        return false;
    }
    return errmsg("Getting info from section type (%s:%d) is not supported\n", GetSectionNameByType(sect_type), sect_type);
}

bool Fs3Operations::IsGetInfoSupported(u_int8_t sect_type)
{
    return GetImageInfoFromSection((u_int8_t*)NULL, sect_type, 1);
}

bool Fs3Operations::IsFs3SectionReadable(u_int8_t type, QueryOptions queryOptions)
{
    //printf("-D- readSectList.size %d\n", (int) _readSectList.size());
    if (_readSectList.size()) {
        for (u_int32_t i = 0; i < _readSectList.size(); i++) {
            if (_readSectList.at(i) == type) {
                return true;
            }
        }
        return false;

    } else if (queryOptions.quickQuery) {
        // TODO: FS3_ROM_CODE , should be part of IsGetInfoSupported ..
        if ( IsGetInfoSupported(type) || type == FS3_ROM_CODE) {
            return true;
        }
        return false;
    }
    return true;
}

bool Fs3Operations::VerifyTOC(u_int32_t dtoc_addr, bool& bad_signature, VerifyCallBack verifyCallBackFunc, bool show_itoc,
        struct QueryOptions queryOptions)
{
    u_int8_t buffer[TOC_HEADER_SIZE], entry_buffer[TOC_ENTRY_SIZE];
    struct cibfw_itoc_header itoc_header;
    bool ret_val = true, mfg_exists = false;
    u_int8_t toc_type = FS3_ITOC;
    u_int32_t phys_addr;
    bad_signature = false;


    // Read the sigmature and check it
    READBUF((*_ioAccess), dtoc_addr, buffer, TOC_HEADER_SIZE, "TOC Header");
    Fs3UpdateImgCache(buffer, dtoc_addr, TOC_HEADER_SIZE);
    cibfw_itoc_header_unpack(&itoc_header, buffer);
    memcpy(_fs3ImgInfo.itocHeader, buffer, CIBFW_ITOC_HEADER_SIZE);
    // cibfw_itoc_header_dump(&itoc_header, stdout);
    u_int32_t first_signature = (toc_type == FS3_ITOC) ? ITOC_ASCII : DTOC_ASCII;
    if (!CheckTocSignature(&itoc_header, first_signature)) {
        bad_signature = true;
        return false;
    }
    u_int32_t toc_crc = CalcImageCRC((u_int32_t*)buffer, (TOC_HEADER_SIZE / 4) - 1);
    phys_addr = _ioAccess->get_phys_from_cont(dtoc_addr, _fwImgInfo.cntxLog2ChunkSize, _fwImgInfo.imgStart != 0);
    if (!DumpFs3CRCCheck(toc_type, phys_addr, TOC_HEADER_SIZE, toc_crc, itoc_header.itoc_entry_crc,false,verifyCallBackFunc)) {
        ret_val = false;
    }
    _fs3ImgInfo.itocAddr = dtoc_addr;

    int section_index = 0;
    struct cibfw_itoc_entry toc_entry;

    do {
        // Uopdate the cont address
        _ioAccess->set_address_convertor(_fwImgInfo.cntxLog2ChunkSize, _fwImgInfo.imgStart != 0);
        u_int32_t entry_addr = dtoc_addr + TOC_HEADER_SIZE + section_index *  TOC_ENTRY_SIZE;
         READBUF((*_ioAccess), entry_addr , entry_buffer, TOC_ENTRY_SIZE, "TOC Entry");
        Fs3UpdateImgCache(entry_buffer, entry_addr, TOC_ENTRY_SIZE);

        cibfw_itoc_entry_unpack(&toc_entry, entry_buffer);
        if (toc_entry.type == FS3_MFG_INFO) {
            mfg_exists = true;
        }
        // printf("-D- toc = %#x, toc_entry.type  = %#x\n", section_index, toc_entry.type);
        if (toc_entry.type != FS3_END) {
            if (section_index + 1 >= MAX_TOCS_NUM) {
                return errmsg("Internal error: number of ITOCs %d is greater than allowed %d", section_index + 1, MAX_TOCS_NUM);
            }

            u_int32_t entry_crc = CalcImageCRC((u_int32_t*)entry_buffer, (TOC_ENTRY_SIZE / 4) - 1);
            u_int32_t entry_size_in_bytes = toc_entry.size * 4;
            //printf("-D- entry_crc = %#x, toc_entry.itoc_entry_crc = %#x\n", entry_crc, toc_entry.itoc_entry_crc);

            if (toc_entry.itoc_entry_crc == entry_crc) {
                // Update last image address
                u_int32_t section_last_addr;
                u_int32_t flash_addr = toc_entry.flash_addr << 2;
                if (!toc_entry.relative_addr) {
                    _ioAccess->set_address_convertor(0, 0);
                    phys_addr = flash_addr;
                    _fs3ImgInfo.smallestAbsAddr = (_fs3ImgInfo.smallestAbsAddr < flash_addr && _fs3ImgInfo.smallestAbsAddr > 0)
                            ? _fs3ImgInfo.smallestAbsAddr : flash_addr;
                } else {
                    phys_addr = _ioAccess->get_phys_from_cont(flash_addr, _fwImgInfo.cntxLog2ChunkSize, _fwImgInfo.imgStart != 0);
                    u_int32_t currSizeOfImgdata = phys_addr + entry_size_in_bytes;
                    _fs3ImgInfo.sizeOfImgData = (_fs3ImgInfo.sizeOfImgData > currSizeOfImgdata) ? _fs3ImgInfo.sizeOfImgData : phys_addr;
                }
                section_last_addr = phys_addr + entry_size_in_bytes;
                _fwImgInfo.lastImageAddr = (_fwImgInfo.lastImageAddr >= phys_addr + section_last_addr) ? _fwImgInfo.lastImageAddr : section_last_addr;

                if (IsFs3SectionReadable(toc_entry.type, queryOptions)) {

                    // Only when we have full verify or the info of this section should be collected for query
                    std::vector<u_int8_t> buffv(entry_size_in_bytes);
                    u_int8_t *buff = (u_int8_t*)(&(buffv[0]));


                    if (show_itoc) {
                        cibfw_itoc_entry_dump(&toc_entry, stdout);
                        DumpFs3CRCCheck(toc_entry.type, phys_addr, entry_size_in_bytes, 0, 0, true, verifyCallBackFunc);
                    } else {
                         READBUF((*_ioAccess), flash_addr, buff, entry_size_in_bytes, "Section");
                        Fs3UpdateImgCache(buff, flash_addr, entry_size_in_bytes);
                        u_int32_t sect_crc = CalcImageCRC((u_int32_t*)buff, toc_entry.size);

                        //printf("-D- flash_addr: %#x, toc_entry_size = %#x, actual sect = %#x, from itoc: %#x\n", flash_addr, toc_entry.size, sect_crc,
                        //        toc_entry.section_crc);
                        if (!DumpFs3CRCCheck(toc_entry.type, phys_addr, entry_size_in_bytes, sect_crc, toc_entry.section_crc, false, verifyCallBackFunc)) {
                            ret_val = false;
                        } else {
                            //printf("-D- toc type : 0x%.8x\n" , toc_entry.type);
                            GetSectData(_fs3ImgInfo.tocArr[section_index].section_data, (u_int32_t*)buff, toc_entry.size * 4);
                            if (IsGetInfoSupported(toc_entry.type)) {
                                 if (!GetImageInfoFromSection(buff, toc_entry.type)) {
                                     return errmsg("Failed to get info from section %d", toc_entry.type);
                                 }
                            } else if (toc_entry.type == FS3_DBG_LOG_MAP) {
                                 TOCPUn(buff, toc_entry.size);
                                 GetSectData(_fwConfSect, (u_int32_t*)buff, toc_entry.size * 4);
                            } else if (toc_entry.type == FS3_ROM_CODE) {
                                // TODO: Need to put the ROM is part of  GetImageInfoFromSection function not alone

                                TOCPUn(buff, toc_entry.size);
                                GetSectData(_romSect, (u_int32_t*)buff, toc_entry.size * 4);
                                RomInfo rInfo(_romSect);
                                rInfo.ParseInfo();
                                rInfo.initRomsInfo(&_fwImgInfo.ext_info.roms_info);

                            }
                        }
                    }
                 }
             } else {

                 // TODO: print crc error
                 /*
                  printf("-D- Bad ITOC CRC: toc_entry.itoc_entry_crc = %#x, actual crc: %#x, entry_size_in_bytes = %#x\n", toc_entry.itoc_entry_crc,
                         entry_crc, entry_size_in_bytes);
                  */
                 ret_val = false;
            }

            _fs3ImgInfo.tocArr[section_index].entry_addr = entry_addr;
            _fs3ImgInfo.tocArr[section_index].toc_entry = toc_entry;
            memcpy(_fs3ImgInfo.tocArr[section_index].data, entry_buffer, CIBFW_ITOC_ENTRY_SIZE);
        }
        section_index++;
    } while (toc_entry.type != FS3_END);
    _fs3ImgInfo.numOfItocs = section_index - 1;
    if (!mfg_exists) {
        return errmsg("No \""MFG_INFO"\" info section.");
    }
    return ret_val;
}


bool Fs3Operations::FwVerify(VerifyCallBack verifyCallBackFunc, bool isStripedImage, bool showItoc) {
    //dummy assignment to avoid compiler warrning (isStripedImage is not used in fs3)
    isStripedImage = true;

    struct QueryOptions queryOptions;
    queryOptions.readRom = true;
    queryOptions.quickQuery = false;

    return Fs3Verify(verifyCallBackFunc, showItoc, queryOptions);
}

bool Fs3Operations::Fs3Verify(VerifyCallBack verifyCallBackFunc, bool show_itoc, struct QueryOptions queryOptions)
{
    u_int32_t cntx_image_start[CNTX_START_POS_SIZE];
    u_int32_t cntx_image_num;
    u_int32_t buff[FS3_BOOT_START_IN_DW];
    u_int32_t offset;
    bool bad_signature;

    // TODO: We need to check this paramater
    _isFullVerify = true;

    CntxFindAllImageStart(_ioAccess, cntx_image_start, &cntx_image_num);
    if (cntx_image_num == 0) {
        return errmsg("No valid FS3 image found");
    }
    u_int32_t image_start = cntx_image_start[0];
    offset = 0;
    // TODO:  Check more than one image
    // Read BOOT
    //f.set_address_convertor(0, 0);
    // Put info
    _fwImgInfo.imgStart = image_start;
    _fwImgInfo.cntxLog2ChunkSize = 21; // TODO: should it be hardcoded
    _fwImgInfo.ext_info.is_failsafe        = true;
    _fwImgInfo.actuallyFailsafe  = true;
    _fwImgInfo.magicPatternFound = 1;

    _ioAccess->set_address_convertor(_fwImgInfo.cntxLog2ChunkSize, _fwImgInfo.imgStart != 0);
    READBUF((*_ioAccess), 0, buff, FS3_BOOT_START, "Image header");
    Fs3UpdateImgCache((u_int8_t*)buff, 0, FS3_BOOT_START);
    TOCPUn(buff, FS3_BOOT_START_IN_DW);



    report_callback(verifyCallBackFunc, "\nFS3 failsafe image\n\n"); // TODO: Do we have non-faisafe image
    // TODO: Get BOOT2 - No need to read all always
    offset += FS2_BOOT_START;
    FS3_CHECKB2(0, offset, true, PRE_CRC_OUTPUT, verifyCallBackFunc);

    offset += _fs3ImgInfo.bootSize;
    _fs3ImgInfo.firstItocIsEmpty = false;
    // printf("-D- image_start = %#x\n", image_start);
    // TODO: Get info of the failsafe from the Image header - no need always failsafe
    // Go over the ITOC entries
    u_int32_t sector_size = (_ioAccess->is_flash()) ? _ioAccess->get_sector_size() : FS3_DEFAULT_SECTOR_SIZE;
    offset = (offset % sector_size == 0) ? offset : (offset + sector_size - offset % 0x1000);

    while (offset < _ioAccess->get_size())
    {
        if (VerifyTOC(offset, bad_signature, verifyCallBackFunc, show_itoc, queryOptions)) {
            return true;
        } else {
            if (!bad_signature) {
                return false;
            }
            _fs3ImgInfo.firstItocIsEmpty = true;

        }
        offset += sector_size;
    }

    return errmsg("No valid ITOC was found.");
}


bool Fs3Operations::Fs3IntQuery(bool readRom, bool quickQuery)
{
    struct QueryOptions queryOptions;
    queryOptions.readRom = readRom;
    queryOptions.quickQuery = quickQuery;
    if (!Fs3Verify((VerifyCallBack)NULL, 0, queryOptions)) {
        return false;
    }
    // works only on device
    _fwImgInfo.ext_info.chip_type = getChipTypeFromHwDevid(_ioAccess->get_dev_id());
    if (_fwImgInfo.ext_info.chip_type == CT_CONNECT_IB) {
        _fwImgInfo.ext_info.dev_type = CONNECT_IB_SW_ID;
    }

    return true;
}

bool Fs3Operations::FwQuery(fw_info_t *fwInfo, bool readRom, bool isStripedImage)
{
    //isStripedImage flag is not needed in FS3 image format
    // Avoid warning - no striped image in FS3
    isStripedImage = false;
    if (!Fs3IntQuery(readRom)) {
        return false;
    }
    memcpy(&(fwInfo->fw_info),  &(_fwImgInfo.ext_info),  sizeof(fw_info_com_t));
    memcpy(&(fwInfo->fs3_info), &(_fs3ImgInfo.ext_info), sizeof(fs3_info_t));
    fwInfo->fw_type = FIT_FS3;
    return true;
}

u_int8_t Fs3Operations::FwType()
{
    return FIT_FS3;
}


bool Fs3Operations::FwInit()
{
    FwInitCom();
    memset(&_fs3ImgInfo, 0, sizeof(_fs3ImgInfo));
    return true;
}


bool Fs3Operations::UpdateDevDataITOC(u_int8_t *image_data, struct toc_info *image_toc_entry, struct toc_info *flash_toc_arr, int flash_toc_size)
{
    u_int8_t itoc_data[CIBFW_ITOC_ENTRY_SIZE];

    for (int i = 0; i < flash_toc_size; i++) {
        struct toc_info *flash_toc_info = &flash_toc_arr[i];
        struct cibfw_itoc_entry *flash_toc_entry = &flash_toc_info->toc_entry;
        if (flash_toc_entry->type == image_toc_entry->toc_entry.type) {
            memset(itoc_data, 0, CIBFW_ITOC_ENTRY_SIZE);
            cibfw_itoc_entry_pack(flash_toc_entry, itoc_data);
            memcpy(&image_data[image_toc_entry->entry_addr], itoc_data, CIBFW_ITOC_ENTRY_SIZE);
            cibfw_itoc_entry_unpack(&image_toc_entry->toc_entry, &image_data[image_toc_entry->entry_addr]);
        }
    }
    return true;
}

#define FS3_FLASH_SIZE 0x400000
bool Fs3Operations::CheckFs3ImgSize(Fs3Operations imageOps)
{
    u_int32_t flashSize = (_ioAccess->is_flash()) ? _ioAccess->get_size() : FS3_FLASH_SIZE;
    u_int32_t maxFsImgSize = flashSize / 2;
    u_int32_t maxImgDataSize = maxFsImgSize - (flashSize - _fs3ImgInfo.smallestAbsAddr);

    if (imageOps._fs3ImgInfo.sizeOfImgData > maxImgDataSize) {
        return errmsg("Size of image data (0x%x) is greater than max size of image data (0x%x)",
                imageOps._fs3ImgInfo.sizeOfImgData,  maxImgDataSize);
    }
    return true;
}

bool Fs3Operations::BurnFs3Image(Fs3Operations &imageOps,
                                  ExtBurnParams& burnParams)
{
    u_int8_t  is_curr_image_in_odd_chunks;
    u_int32_t new_image_start, image_size = 0;

    Flash    *f     = (Flash*)(this->_ioAccess);
    FImage   *fim   = (FImage*)(imageOps._ioAccess);
    u_int8_t *data8 = (u_int8_t *) fim->getBuf();


    if (_fwImgInfo.imgStart != 0) {
        is_curr_image_in_odd_chunks = 1;
        new_image_start = 0;
    } else {
        is_curr_image_in_odd_chunks = 0;
        new_image_start = (1 << imageOps._fwImgInfo.cntxLog2ChunkSize);
    }
    /*printf("-D- new_image_start = %#x, is_curr_image_in_odd_chunks = %#x\n", new_image_start, is_curr_image_in_odd_chunks);
    printf("-D- num_of_itocs = %d\n", image_info->num_of_itocs);
    */
    if (!CheckFs3ImgSize(imageOps)) {
        return false;
    }

    f->set_address_convertor(imageOps._fwImgInfo.cntxLog2ChunkSize, !is_curr_image_in_odd_chunks);

    for (int i = 0; i < imageOps._fs3ImgInfo.numOfItocs; i++) {
        struct toc_info *itoc_info_p = &imageOps._fs3ImgInfo.tocArr[i];
        struct cibfw_itoc_entry *toc_entry = &itoc_info_p->toc_entry;
        // printf("-D- itoc_addr = %#x\n", itoc_info_p->entry_addr);
        if (toc_entry->device_data) {
            // TODO: Copy the ITOC from Device
            // printf("-D- toc_entry: %#x\n", toc_entry->itoc_entry_crc);
            if (!UpdateDevDataITOC(data8, itoc_info_p, _fs3ImgInfo.tocArr, _fs3ImgInfo.numOfItocs)) {
                return false;
            }
            // printf("-D- After toc_entry: %#x\n", toc_entry->itoc_entry_crc);
        } else {
            u_int32_t last_addr_of_itoc = (toc_entry->flash_addr + toc_entry->size) << 2;
            image_size = (last_addr_of_itoc > image_size) ? last_addr_of_itoc : image_size;
        }

    }


    // TODO: Check that the dev data is not been overridden
    {

         // TODO : remove these remnants of the old printing system , we use callback func now 
         char * pre_message = (char*)"Burning FS3 FW image without signatures";
         u_int32_t  zeroes     = 0;
         char message[128], message1[128], buff[128];
         int allow_nofs = 0;

         if (pre_message == NULL) {
             sprintf(message, "Burning FW image without signatures");
         } else {
             sprintf(message, pre_message);
         }
         int str_len = strlen(message), restore_len = strlen(RESTORING_MSG);
         str_len = (restore_len > str_len) ? restore_len : str_len;

         sprintf(buff, "%%-%ds  - ", str_len);

         sprintf(message1, buff,  message);


         if (!writeImage(burnParams.progressFunc, 16 , data8 + 16, image_size - 16)) {
             return false;
         }
         if (!f->is_flash()) {
             return true;
         }
         // Write new signature
         if (!f->write(0, data8, 16, true)) {
             return false;
         }
         bool boot_address_was_updated = true;
         // Write new image start address to crspace (for SW reset)
         if (!f->update_boot_addr(new_image_start)) {
             boot_address_was_updated = false;
         }

         if (imageOps._fwImgInfo.ext_info.is_failsafe) {
             if (allow_nofs) {
                 // When burning in nofs, remnant of older image with different chunk size
                 // may reside on the flash -
                 // Invalidate all images marking on flash except the one we've just burnt

                 u_int32_t cntx_image_start[CNTX_START_POS_SIZE];
                 u_int32_t cntx_image_num;
                 u_int32_t i;

                 CntxFindAllImageStart(f, cntx_image_start, &cntx_image_num);
                 // Address convertor is disabled now - use phys addresses
                 for (i = 0; i < cntx_image_num; i++) {
                     if (cntx_image_start[i] != new_image_start) {
                         if (!f->write(cntx_image_start[i], &zeroes, sizeof(zeroes), true)) {
                             return false;
                         }
                     }
                 }
             } else {
                 // invalidate previous signature
                 f->set_address_convertor(imageOps._fwImgInfo.cntxLog2ChunkSize, is_curr_image_in_odd_chunks);
                 if (!f->write(0, &zeroes, sizeof(zeroes), true)) {
                     return false;
                 }
             }
         }
         if (boot_address_was_updated == false) {
             report_warn("Failed to update FW boot address. Power cycle the device in order to load the new FW.\n");
         }

    }
    // TODO: Update the signature

    return true;
}
bool Fs3Operations::Fs3Burn(Fs3Operations &imageOps, ExtBurnParams& burnParams)
{

    if (imageOps.FwType() != FIT_FS3) {
        return errmsg("FW image type is not FS3\n");
    }
    if (!Fs3IntQuery()) {
        return false;
    }
    // for image we execute full verify to bring all the information needed for ROM Patch
    if (!imageOps.Fs3IntQuery(true, false)) {
          return false;
    }
    if (!CheckPSID(imageOps, burnParams.allowPsidChange)) {
        return false;
    }

    // Check if the burnt FW version is OK
    if (!CheckFwVersion(imageOps, burnParams.ignoreVersionCheck)) {
        // special_ret_val = RC_FW_ALREADY_UPDATED;
        return false;
    }

    Fs3Operations *imgToBurn = &imageOps;

    // ROM patchs
    if (((burnParams.burnRomOptions == ExtBurnParams::BRO_FROM_DEV_IF_EXIST) && (_fwImgInfo.ext_info.roms_info.exp_rom_found)) || // There is ROM in device and user choses to keep it
            ((burnParams.burnRomOptions == ExtBurnParams::BRO_DEFAULT) && (!imageOps._fwImgInfo.ext_info.roms_info.exp_rom_found && _fwImgInfo.ext_info.roms_info.exp_rom_found))) { // No ROM in image and ROM in device
        // here we should take rom from device and insert into the image
        // i.e if we have rom in image remove it and put the rom from the device else just put rom from device.

        // 1. use Fs3ModifySection to integrate _romSect buff with the image , newImageData contains the modified image buffer
        std::vector<u_int8_t> newImageData(imageOps._fwImgInfo.lastImageAddr);
        std::vector<u_int8_t> romSect = _romSect;
        TOCPUn((u_int32_t*)&romSect[0], romSect.size()/4);
        if (!imageOps.Fs3ReplaceSectionInDevImg(FS3_ROM_CODE, FS3_PCI_CODE, true, (u_int8_t*)&newImageData[0], imageOps._fwImgInfo.lastImageAddr,
            (u_int32_t*)&romSect[0], (u_int32_t)romSect.size())) {
            return errmsg("failed to update ROM in image. %s", imageOps.err());
        }
        // 2. create fs3Operation Obj (handl type BUFF)
        // open the image buffer
        FwOperations* imageWithRomOps = FwOperationsCreate((void*)&newImageData[0], (void*)&imageOps._fwImgInfo.lastImageAddr, (char*)NULL, FHT_FW_BUFF, (char*)NULL, (int)NULL);
        if (!imageWithRomOps) {
            return errmsg("Internal error: The prepared image is corrupted.");
        }
        // 3. verify it
        if (!((Fs3Operations*)imageWithRomOps)->Fs3IntQuery(true,false)) {
            errmsg("Internal error: The prepared image is corrupted: %s", imageWithRomOps->err());
            imageWithRomOps->FwCleanUp();
            delete imageWithRomOps;
            return false;
        }
        // 4. pass it to BurnFs3Image instead of imageOps
        imgToBurn = (Fs3Operations*)imageWithRomOps;
    }

    bool rc = BurnFs3Image(*imgToBurn, burnParams);
    if (imgToBurn != &imageOps) {
        imgToBurn->FwCleanUp();
        delete imgToBurn;
    }
    return rc;
}

bool Fs3Operations::FwBurn(FwOperations *imageOps, u_int8_t forceVersion, ProgressCallBack progressFunc)
{
    if (imageOps == NULL) {
        return errmsg("bad parameter is given to FwBurn\n");
    }

    ExtBurnParams burnParams = ExtBurnParams();
    burnParams.ignoreVersionCheck = forceVersion;
    burnParams.progressFunc = progressFunc;

    return Fs3Burn((*(Fs3Operations*)imageOps), burnParams);
}

bool Fs3Operations::FwBurnAdvanced(FwOperations *imageOps, ExtBurnParams& burnParams)
{
    if (imageOps == NULL) {
        return errmsg("bad parameter is given to FwBurnAdvanced\n");
    }
    return Fs3Burn((*(Fs3Operations*)imageOps), burnParams);
}

bool Fs3Operations::FwBurnBlock(FwOperations *imageOps, ProgressCallBack progressFunc)
{
    // Avoid Warning!
    imageOps = (FwOperations*)NULL;
    progressFunc = (ProgressCallBack)NULL;
    return errmsg("FwBurnBlock is not supported anymore in FS3 image.");
}

bool Fs3Operations::FwReadData(void* image, u_int32_t* imageSize)
{
    struct QueryOptions queryOptions;
    if (!imageSize) {
        return errmsg("bad parameter is given to FwReadData\n");
    }

    queryOptions.readRom = true;
    queryOptions.quickQuery = false;
    if (image == NULL) {
        // When we need only to get size, no need for reading entire image
        queryOptions.readRom = false;
        queryOptions.quickQuery = true;
    }
    // Avoid Warning
    if (!Fs3Verify((VerifyCallBack)NULL, 0, queryOptions)) {
        return false;
    }

    if (image != NULL) {
        memcpy(image,  &_fs3ImgInfo.imageCache[0], _fwImgInfo.lastImageAddr);
    }
    *imageSize = _fwImgInfo.lastImageAddr;
    return true;
}


bool Fs3Operations::FwTest(u_int32_t *data)
{
    _ioAccess->set_address_convertor(0,0);
    if (!_ioAccess->read(0, data)) {
        return false;
    }
    /*

    if (!((Flash*)_ioAccess)->write(0, 0x12345678)) {
        return false;
    }

    if (!_ioAccess->read(0x0, data)) {
        return false;
    }
    */
    return true;
}

bool Fs3Operations::FwReadRom(std::vector<u_int8_t>& romSect)
{
    if (!Fs3IntQuery()) {
        return false;
    }
    if (_romSect.empty()) {
        return errmsg("Read ROM failed: The FW does not contain a ROM section");
    }
    romSect = _romSect;
    // Set endianness
    TOCPUn(&(romSect[0]), romSect.size()/4);
    return true;
}

bool Fs3Operations::FwGetSection (u_int32_t sectType, std::vector<u_int8_t>& sectInfo)
{
    //we treat H_FW_CONF as FS3_DBG_LOG_MAP
    //FwGetSection only supports retrieving FS3_DBG_LOG_MAP section atm.
    if (sectType != H_FW_CONF && sectType != FS3_DBG_LOG_MAP) {
        return errmsg("Hash File section not found in the given image.");
    }
    //set the sector to read (need to remove it after read)
    _readSectList.push_back(FS3_DBG_LOG_MAP);
    if (!Fs3IntQuery()) {
        _readSectList.pop_back();
        return false;
    }
    _readSectList.pop_back();
    sectInfo = _fwConfSect;
    if (sectInfo.empty()) {
        return errmsg("Hash File section not found in the given image.");
    }
    return true;
}


bool Fs3Operations::FwSetMFG(guid_t baseGuid, PrintCallBack callBackFunc)
{

    if (!Fs3UpdateSection(&baseGuid, FS3_MFG_INFO, false, CMD_SET_MFG_GUIDS, callBackFunc)) {
        return false;
    }
    return true;
}

bool Fs3Operations::FwSetGuids(sg_params_t& sgParam, PrintCallBack callBackFunc, ProgressCallBack progressFunc)
{
    // Avoid Warning because there is no need for progressFunc
    progressFunc = (ProgressCallBack)NULL;
    if (sgParam.userGuids.empty()) {
        return errmsg("Base GUID not found.");
    }
    if (!Fs3UpdateSection(&sgParam.userGuids[0], FS3_DEV_INFO, true, CMD_SET_GUIDS, callBackFunc)) {
        return false;
    }
    return true;
}

bool Fs3Operations::FwSetVPD(char* vpdFileStr, PrintCallBack callBackFunc)
{
    if (!vpdFileStr) {
        return errmsg("Please specify a valid vpd file.");
    }

    if (!Fs3UpdateSection(vpdFileStr, FS3_VPD_R0, false, CMD_BURN_VPD, callBackFunc)) {
        return false;
    }
    return true;
}

bool Fs3Operations::GetModifiedSectionInfo(fs3_section_t sectionType, fs3_section_t nextSectionType, u_int32_t &newSectAddr,
        fs3_section_t &SectToPut, u_int32_t &oldSectSize)
{
    struct toc_info *curr_itoc;
    if (Fs3GetItocInfo(_fs3ImgInfo.tocArr, _fs3ImgInfo.numOfItocs, sectionType, curr_itoc) ||
        Fs3GetItocInfo(_fs3ImgInfo.tocArr, _fs3ImgInfo.numOfItocs, nextSectionType, curr_itoc)) {
        newSectAddr = curr_itoc->toc_entry.flash_addr << 2;
        SectToPut = (curr_itoc->toc_entry.type == sectionType) ? sectionType :  nextSectionType;
        oldSectSize = curr_itoc->toc_entry.size * 4;
        return true;
    }
    return false;
}

bool Fs3Operations::ShiftItocAddrInEntry(struct toc_info *newItocInfo, struct toc_info *oldItocInfo, int shiftSize)
{
    u_int32_t currSectaddr;
    CopyItocInfo(newItocInfo, oldItocInfo);
    currSectaddr =  (newItocInfo->toc_entry.flash_addr << 2) + shiftSize;
    Fs3UpdateItocInfo(newItocInfo, currSectaddr);
    return true;
}
bool Fs3Operations::Fs3UpdateItocInfo(struct toc_info *newItocInfo, u_int32_t newSectAddr, fs3_section_t sectionType, u_int32_t* newSectData,
        u_int32_t NewSectSize)
{
    std::vector<u_int8_t>  newSecVect(NewSectSize);
    newItocInfo->toc_entry.type = sectionType;
    memcpy(&newSecVect[0], newSectData, NewSectSize);
    return Fs3UpdateItocInfo(newItocInfo, newSectAddr, NewSectSize / 4, newSecVect);
}


bool Fs3Operations::CopyItocInfo(struct toc_info *newTocInfo, struct toc_info *currToc)
{
    memcpy(newTocInfo->data, currToc->data, CIBFW_ITOC_ENTRY_SIZE);
    newTocInfo->entry_addr = currToc->entry_addr;
    newTocInfo->section_data = currToc->section_data;
    newTocInfo->toc_entry = currToc->toc_entry;
    return true;
}




bool Fs3Operations::UpdateItocAfterInsert(fs3_section_t sectionType, u_int32_t newSectAddr, fs3_section_t SectToPut, bool toAdd, u_int32_t* newSectData,
        u_int32_t removedOrNewSectSize, struct toc_info *tocArr, u_int32_t &numOfItocs)
{
    bool isReplacement = (sectionType == SectToPut) ? true : false;
    int shiftSize;

    if (toAdd) {
        if (isReplacement) {
            struct toc_info *curr_itoc;
            u_int32_t sectSize;
            Fs3GetItocInfo(_fs3ImgInfo.tocArr, _fs3ImgInfo.numOfItocs, sectionType, curr_itoc);
            sectSize = curr_itoc->toc_entry.size * 4;
            shiftSize = (removedOrNewSectSize > sectSize) ? removedOrNewSectSize - sectSize : 0;
        } else {
            shiftSize = removedOrNewSectSize;
        }
        if (shiftSize % FS3_DEFAULT_SECTOR_SIZE) {
            shiftSize += (FS3_DEFAULT_SECTOR_SIZE - shiftSize % FS3_DEFAULT_SECTOR_SIZE);
        }
    } else {
        shiftSize = 0;
        if (removedOrNewSectSize % FS3_DEFAULT_SECTOR_SIZE) {
            removedOrNewSectSize += (FS3_DEFAULT_SECTOR_SIZE - removedOrNewSectSize % FS3_DEFAULT_SECTOR_SIZE);
        }
        shiftSize -= removedOrNewSectSize;
    }
    numOfItocs = 0;
    for (int i = 0; i < _fs3ImgInfo.numOfItocs; i++) {
        struct toc_info *curr_itoc = &_fs3ImgInfo.tocArr[i];
        struct toc_info *newTocInfo = &tocArr[numOfItocs];
        u_int32_t currSectaddr =  curr_itoc->toc_entry.flash_addr << 2; // Put it in one place

        if (currSectaddr > newSectAddr) {
            if (!curr_itoc->toc_entry.relative_addr) {

                CopyItocInfo(newTocInfo, curr_itoc);
            } else {
                ShiftItocAddrInEntry(newTocInfo, curr_itoc, shiftSize);
            }
        } else if (currSectaddr == newSectAddr) {
            if (!toAdd) {
                continue;
            }
            CopyItocInfo(newTocInfo, curr_itoc);
            Fs3UpdateItocInfo(newTocInfo, newSectAddr, sectionType, newSectData, removedOrNewSectSize);

            if (!isReplacement) {
                // put next section
                newTocInfo = &tocArr[++numOfItocs];
                ShiftItocAddrInEntry(newTocInfo, curr_itoc, shiftSize);
            }
        } else {
             // just Copy the ITOC as is
            CopyItocInfo(newTocInfo, curr_itoc);
        }
        numOfItocs++;
    }
    return true;
}

bool Fs3Operations::UpdateImageAfterInsert(struct toc_info *tocArr, u_int32_t numOfItocs, u_int8_t* newImgData, u_int32_t newImageSize)
{


    // Copy data before itocAddr and ITOC header
    memcpy(newImgData, &_fs3ImgInfo.imageCache[0], _fs3ImgInfo.itocAddr);
    memcpy(&newImgData[_fs3ImgInfo.itocAddr], _fs3ImgInfo.itocHeader, CIBFW_ITOC_HEADER_SIZE);
    for (int i = 0; i < (int)numOfItocs; i++) {
        // Inits
        u_int32_t itocOffset = _fs3ImgInfo.itocAddr + CIBFW_ITOC_HEADER_SIZE + i * CIBFW_ITOC_ENTRY_SIZE;
        struct toc_info *currItoc = &tocArr[i];
        u_int8_t sectType = currItoc->toc_entry.type;
        // TODO: What should we do with Itoc Addr
        u_int32_t sectAddr = currItoc->toc_entry.flash_addr << 2;
        u_int32_t sectSize = currItoc->toc_entry.size * 4;
        // Some checks
        if (sectAddr + sectSize > newImageSize) {
            return errmsg("Internal error: Size of modified image (0x%x) is longer than size of original image (0x%x)!", sectAddr + sectSize, newImageSize);
        }
        if (sectSize != currItoc->section_data.size()) {
            return errmsg("Internal error: Sectoion size of %s (0x%x) is not equal to allocated memory for it(0x%x)", GetSectionNameByType(sectType),
                    sectSize, (u_int32_t)currItoc->section_data.size());
        }

        memcpy(&newImgData[itocOffset], currItoc->data, CIBFW_ITOC_ENTRY_SIZE);

        memcpy(&newImgData[sectAddr], &currItoc->section_data[0], sectSize);
    }
    u_int32_t lastItocSect = _fs3ImgInfo.itocAddr + CIBFW_ITOC_HEADER_SIZE + numOfItocs * CIBFW_ITOC_ENTRY_SIZE;
    memset(&newImgData[lastItocSect], FS3_END, CIBFW_ITOC_ENTRY_SIZE);
    return true;
}

bool Fs3Operations::Fs3ReplaceSectionInDevImg(fs3_section_t sectionType, fs3_section_t nextSectionType, bool toAdd, u_int8_t* newImgData, u_int32_t newImageSize,
        u_int32_t* newSectData, u_int32_t NewSectSize)
{
    u_int32_t newSectAddr;
    u_int32_t numOfItocs;
    struct toc_info tocArr[MAX_TOCS_NUM];
    fs3_section_t sectToPut;
    u_int32_t oldSectSize;

    if (!GetModifiedSectionInfo(sectionType, nextSectionType, newSectAddr, sectToPut, oldSectSize)) {
        return false;
    }
    u_int32_t removedOrNewSectSize = (toAdd) ? NewSectSize : oldSectSize;

    if (!UpdateItocAfterInsert(sectionType, newSectAddr, sectToPut, toAdd, newSectData, removedOrNewSectSize, tocArr, numOfItocs)) {
        return false;
    }
    if (!UpdateImageAfterInsert(tocArr, numOfItocs, newImgData, newImageSize)) {
        return false;
    }
    return true;
}

bool Fs3Operations::Fs3ModifySection(fs3_section_t sectionType, fs3_section_t neighbourSection, bool toAdd, u_int32_t* newSectData, u_int32_t newSectSize,
        ProgressCallBack progressFunc)
{
    // Get image data and ROM data and integrate ROM data into image data
    // Verify FW on device
    if (!FwVerify((VerifyCallBack)NULL)) {
        return errmsg("Verify FW burn on the device failed: %s", err());
    }

    std::vector<u_int8_t> newImageData(_fwImgInfo.lastImageAddr);
    // u_int8_t *newImageData = new u_int8_t[_fwImgInfo.lastImageAddr];

    if (!Fs3ReplaceSectionInDevImg(sectionType, neighbourSection, toAdd, (u_int8_t*)&newImageData[0], _fwImgInfo.lastImageAddr,
            newSectData, newSectSize)) {
        ///delete[] newImageData;
        return false;
    }
    // Burn the new image into the device.
    if (!FwBurnData((u_int32_t*)&newImageData[0], _fwImgInfo.lastImageAddr, progressFunc)) {
        return false;
    }
    return true;
}

bool Fs3Operations::Fs3AddSection(fs3_section_t sectionType, fs3_section_t neighbourSection, u_int32_t* newSectData, u_int32_t newSectSize,
        ProgressCallBack progressFunc)
{
    // We need to add the new section before the neighbourSection
    return Fs3ModifySection(sectionType, neighbourSection, true, newSectData, newSectSize, progressFunc);
}

bool Fs3Operations::Fs3RemoveSection(fs3_section_t sectionType, ProgressCallBack progressFunc)
{
    return Fs3ModifySection(sectionType, sectionType, false, NULL, 0, progressFunc);
}


bool Fs3Operations::FwBurnRom(FImage* romImg, bool ignoreProdIdCheck, bool ignoreDevidCheck,
        ProgressCallBack progressFunc)
{
    roms_info_t romsInfo;

    if (romImg == NULL) {
        return errmsg("Bad ROM image is given.");
    }

    if (romImg->getBufLength() == 0) {
        return errmsg("Bad ROM file: Empty file.");
    }
    if (!FwOperations::getRomsInfo(romImg, romsInfo)) {
            return errmsg("Failed to read given ROM.");
    }
    if (!Fs3IntQuery(false)) {
        return false;
    }

    if (!ignoreProdIdCheck && strcmp(_fwImgInfo.ext_info.product_ver, "") != 0) {
        return errmsg("The device FW contains common FW/ROM Product Version - The ROM cannot be updated separately.");
    }

    if (!ignoreDevidCheck && !FwOperations::checkMatchingExpRomDevId(_fwImgInfo.ext_info.dev_type, romsInfo)) {
        return errmsg("Image file ROM: FW is for device %d, but Exp-ROM is for device %d\n", _fwImgInfo.ext_info.dev_type,
                romsInfo.exp_rom_com_devid);
    }
    return Fs3AddSection(FS3_ROM_CODE, FS3_PCI_CODE, romImg->getBuf(), romImg->getBufLength(), progressFunc);
}

bool Fs3Operations::FwDeleteRom(bool ignoreProdIdCheck, ProgressCallBack progressFunc)
{
    //run int query to get product ver
    if (!Fs3IntQuery(true)) {
        return false;
    }

    if (_romSect.empty()) {
        return errmsg("The FW does not contain a ROM section");
    }

    if (!ignoreProdIdCheck && strcmp(_fwImgInfo.ext_info.product_ver, "") != 0) {
        return errmsg("The device FW contains common FW/ROM Product Version - The ROM cannot be updated separately.");
    }

    return Fs3RemoveSection(FS3_ROM_CODE, progressFunc);
}

bool Fs3Operations::Fs3GetItocInfo(struct toc_info *tocArr, int num_of_itocs, fs3_section_t sect_type, struct toc_info *&curr_toc)
{
    for (int i = 0; i < num_of_itocs; i++) {
        struct toc_info *itoc_info = &tocArr[i];
        if (itoc_info->toc_entry.type == sect_type) {
            curr_toc =  itoc_info;
            return true;
        }
    }
    return errmsg("ITOC entry type: %s (%d) not found", GetSectionNameByType(sect_type), sect_type);
}

bool Fs3Operations::Fs3UpdateMfgUidsSection(struct toc_info *curr_toc, std::vector<u_int8_t>  section_data, guid_t base_uid, 
                                            std::vector<u_int8_t>  &newSectionData)
{
    struct cibfw_mfg_info mfg_info;
    cibfw_mfg_info_unpack(&mfg_info, (u_int8_t*)&section_data[0]);

    Fs3ChangeUidsFromBase(base_uid, &mfg_info.guids);
    newSectionData = section_data;
    memset((u_int8_t*)&newSectionData[0], 0, curr_toc->toc_entry.size * 4);
    cibfw_mfg_info_pack(&mfg_info, (u_int8_t*)&newSectionData[0]);
    return true;
}

bool Fs3Operations::Fs3ChangeUidsFromBase(guid_t base_uid, struct cibfw_guids *guids)
{
    u_int64_t base_uid_64 =  base_uid.l | (u_int64_t)base_uid.h << 32;
    // Should be put somewhere in common place
    u_int64_t base_mac_64 =  ((u_int64_t)base_uid.l & 0xffffff) | (((u_int64_t)base_uid.h & 0xffffff00) << 16);

    guids->guids[0].uid = base_uid_64;
    guids->guids[1].uid = base_uid_64 + 8;
    guids->macs[0].uid = base_mac_64;
    guids->macs[1].uid = base_mac_64 + 8;
    return true;
}

bool Fs3Operations::Fs3UpdateUidsSection(struct toc_info *curr_toc, std::vector<u_int8_t>  section_data, guid_t base_uid,
        std::vector<u_int8_t>  &newSectionData)
{
    struct cibfw_device_info dev_info;
    cibfw_device_info_unpack(&dev_info, (u_int8_t*)&section_data[0]);
    Fs3ChangeUidsFromBase(base_uid, &dev_info.guids);
    newSectionData = section_data;
    memset((u_int8_t*)&newSectionData[0], 0, curr_toc->toc_entry.size * 4);
    cibfw_device_info_pack(&dev_info, (u_int8_t*)&newSectionData[0]);
    return true;
}

bool Fs3Operations::Fs3UpdateVsdSection(struct toc_info *curr_toc, std::vector<u_int8_t>  section_data, char* user_vsd,
        std::vector<u_int8_t>  &newSectionData)
{
    struct cibfw_device_info dev_info;
    cibfw_device_info_unpack(&dev_info, (u_int8_t*)&section_data[0]);
    size_t len = strlen(user_vsd);
    if (len > VSD_LEN-1) {
        return errmsg("VSD too long: %d", (int)len);
    }
    strcpy(dev_info.vsd, user_vsd);
    newSectionData = section_data;
    memset((u_int8_t*)&newSectionData[0], 0, curr_toc->toc_entry.size * 4);
    cibfw_device_info_pack(&dev_info, (u_int8_t*)&newSectionData[0]);
    return true;
}

bool Fs3Operations::Fs3UpdateVpdSection(struct toc_info *curr_toc, char *vpd,
                               std::vector<u_int8_t>  &newSectionData)
{
    int vpd_size;
    u_int8_t *vpd_data;

    if (!ReadImageFile(vpd, vpd_data, vpd_size)) {
        return false;
    }
    if (vpd_size % 4) {
        return errmsg("Size of VPD file: %d is not 4-byte alligned!", vpd_size);
    }
    GetSectData(newSectionData, (u_int32_t*)vpd_data, vpd_size);
    curr_toc->toc_entry.size = vpd_size / 4;
    delete[] vpd_data;
    return true;
}

bool Fs3Operations::Fs3GetNewSectionAddr(struct toc_info *tocArr, struct toc_info *curr_toc, u_int32_t &NewSectionAddr, bool failsafe_section)
{
    // printf("-D- addr  = %#x\n", curr_toc->toc_entry.flash_addr);
    u_int32_t sector_size = (_ioAccess->is_flash()) ? _ioAccess->get_sector_size() : FS3_DEFAULT_SECTOR_SIZE;
    u_int32_t flash_addr = curr_toc->toc_entry.flash_addr << 2;
    // HACK: THIS IS AN UGLY HACK, SHOULD BE REMOVED ASAP
    if (failsafe_section) {
        NewSectionAddr = (flash_addr == 0x3fd000) ? 0x3fe000 : 0x3fd000;
    } else {
        NewSectionAddr = flash_addr;
    }
    // HACK: Avoid warning
    tocArr = NULL;
    // fbase = 0;
    sector_size = 0;
    return true;
}

bool Fs3Operations::CalcItocEntryCRC(struct toc_info *curr_toc)
{
    u_int8_t  new_entry_data[CIBFW_ITOC_ENTRY_SIZE];
    memset(new_entry_data, 0, CIBFW_ITOC_ENTRY_SIZE);

    cibfw_itoc_entry_pack(&curr_toc->toc_entry, new_entry_data);
    u_int32_t entry_crc = CalcImageCRC((u_int32_t*)new_entry_data, (TOC_ENTRY_SIZE / 4) - 1);
    curr_toc->toc_entry.itoc_entry_crc = entry_crc;
    return true;

}

bool Fs3Operations::Fs3UpdateItocData(struct toc_info *currToc)
{
    CalcItocEntryCRC(currToc);
    memset(currToc->data, 0, CIBFW_ITOC_ENTRY_SIZE);
    cibfw_itoc_entry_pack(&currToc->toc_entry, currToc->data);

    return true;

}

bool Fs3Operations::Fs3UpdateItocInfo(struct toc_info *curr_toc, u_int32_t newSectionAddr)
{
    // We assume it's absolute
     curr_toc->toc_entry.flash_addr = newSectionAddr >> 2;
     return Fs3UpdateItocData(curr_toc);

}
bool Fs3Operations::Fs3UpdateItocInfo(struct toc_info *curr_toc, u_int32_t newSectionAddr, u_int32_t NewSectSize, std::vector<u_int8_t>  newSectionData)
{
    curr_toc->section_data = newSectionData;
    curr_toc->toc_entry.size = NewSectSize;
    u_int32_t new_crc = CalcImageCRC((u_int32_t*)&newSectionData[0], curr_toc->toc_entry.size);
    curr_toc->toc_entry.section_crc = new_crc;

    return Fs3UpdateItocInfo(curr_toc, newSectionAddr);
 }

bool Fs3Operations::Fs3ReburnItocSection(u_int32_t newSectionAddr,
        u_int32_t newSectionSize, std::vector<u_int8_t>  newSectionData, char *msg, PrintCallBack callBackFunc)
{

    // HACK SHOULD BE REMOVED ASAP
    u_int32_t sector_size = (_ioAccess->is_flash()) ? _ioAccess->get_sector_size() : FS3_DEFAULT_SECTOR_SIZE;
    u_int32_t oldItocAddr = _fs3ImgInfo.itocAddr;
    u_int32_t newItocAddr = (_fs3ImgInfo.firstItocIsEmpty) ? (_fs3ImgInfo.itocAddr - sector_size) :  (_fs3ImgInfo.itocAddr + sector_size);
    char message[127];

    sprintf(message, "Updating %-4s section - ", msg);
    // Burn new Section
    // we pass a null callback and print the progress here as the writes are small (guids/mfg/vpd_str)
    // in the future if we want to pass the cb prints to writeImage , need to change the signature of progressCallBack to recieve and optional string to print
    if (callBackFunc) {
        callBackFunc(message);
    }
    if (!writeImage((ProgressCallBack)NULL, newSectionAddr , (u_int8_t*)&newSectionData[0], newSectionSize, true)) {
        if (callBackFunc) {
            callBackFunc("FAILED\n");
        }
        return false;
    }
    if (callBackFunc) {
        callBackFunc("OK\n");
    }
    // Update new ITOC
    std::vector<u_int8_t>  itocEntriesData;
    u_int32_t itocSize = (_fs3ImgInfo.numOfItocs + 1 ) * CIBFW_ITOC_ENTRY_SIZE + CIBFW_ITOC_HEADER_SIZE;
    u_int8_t *p = itocEntriesData.get_allocator().allocate(itocSize);
    memcpy(p, _fs3ImgInfo.itocHeader, CIBFW_ITOC_HEADER_SIZE);
    for (int i = 0; i < _fs3ImgInfo.numOfItocs; i++) {
        struct toc_info *curr_itoc = &_fs3ImgInfo.tocArr[i];
        memcpy(p + CIBFW_ITOC_HEADER_SIZE + i * CIBFW_ITOC_ENTRY_SIZE, curr_itoc->data, CIBFW_ITOC_ENTRY_SIZE);
    }
    memset(&p[itocSize] - CIBFW_ITOC_ENTRY_SIZE, FS3_END, CIBFW_ITOC_ENTRY_SIZE);


    if (callBackFunc) {
        callBackFunc("Updating ITOC section - ");
    }
    if (!writeImage((ProgressCallBack)NULL, newItocAddr , p, itocSize, false)) {
        if (callBackFunc) {
            callBackFunc("FAILED\n");
        }
        return false;
    }
    if (callBackFunc) {
        callBackFunc("OK\n");
    }
    u_int32_t zeros = 0;

    if (callBackFunc) {
        callBackFunc("Restoring signature   - ");
    }
    if (!writeImage((ProgressCallBack)NULL, oldItocAddr, (u_int8_t*)&zeros, 4, false)) {
        if (callBackFunc) {
            callBackFunc("FAILED\n");
        }
        return false;
    }
    if (callBackFunc) {
        callBackFunc("OK\n");
    }
    return true;
}

//add callback if we want info during section update
bool  Fs3Operations::Fs3UpdateSection(void *new_info, fs3_section_t sect_type, bool is_sect_failsafe, CommandType cmd_type, PrintCallBack callBackFunc)
{
    struct toc_info *newTocArr;
    struct toc_info *curr_toc;
    std::vector<u_int8_t> newUidSection;
    u_int32_t newSectionAddr;
    char *type_msg;
    // init sector to read
    _readSectList.push_back(sect_type);
    if (!Fs3IntQuery()) {
        _readSectList.pop_back();
        return false;
    }
    _readSectList.pop_back();
    // _silent = curr_silent;

    // get newTocArr
    newTocArr = _fs3ImgInfo.tocArr;

    if (!Fs3GetItocInfo(_fs3ImgInfo.tocArr, _fs3ImgInfo.numOfItocs, sect_type, curr_toc)) {
         return false;
     }

    if (sect_type == FS3_MFG_INFO) {
        guid_t base_uid = *(guid_t*)new_info;
        type_msg = (char*)"GUID";
        if (!Fs3UpdateMfgUidsSection(curr_toc, curr_toc->section_data, base_uid, newUidSection)) {
            return false;
        }
    } else if (sect_type == FS3_DEV_INFO) {
        if (cmd_type == CMD_SET_GUIDS) {
            guid_t base_uid = *(guid_t*)new_info;
            type_msg = (char*)"GUID";
            if (!Fs3UpdateUidsSection(curr_toc, curr_toc->section_data, base_uid, newUidSection)) {
                return false;
            }
        } else if(cmd_type == CMD_SET_VSD) {
            char* user_vsd = (char*)new_info;
            type_msg = (char*)"VSD";
            if (!Fs3UpdateVsdSection(curr_toc, curr_toc->section_data, user_vsd, newUidSection)) {
                return false;
            }
            } else {
                // We shouldnt reach here EVER
                type_msg = (char*)"Unknown";
        }
    } else if (sect_type == FS3_VPD_R0) {
        char *vpd_file = (char*)new_info;
        type_msg = (char*)"VPD";
        if (!Fs3UpdateVpdSection(curr_toc, vpd_file, newUidSection)) {
            return false;
        }
    } else {
        return errmsg("Section type %s is not supported\n", GetSectionNameByType(sect_type));
    }


    if (!Fs3GetNewSectionAddr(_fs3ImgInfo.tocArr, curr_toc, newSectionAddr, is_sect_failsafe)) {
        return false;
    }
    if (!Fs3UpdateItocInfo(curr_toc, newSectionAddr, curr_toc->toc_entry.size, newUidSection)) {
        return false;
    }
    if (!Fs3ReburnItocSection(newSectionAddr, curr_toc->toc_entry.size * 4, newUidSection, type_msg, callBackFunc)) {
        return false;
    }
    return true;
}


bool Fs3Operations::FwSetVSD(char* vsdStr, ProgressCallBack progressFunc, PrintCallBack printFunc)
{
    // Avoid warning
    progressFunc = (ProgressCallBack)NULL;
    if (!vsdStr) {
        return errmsg("Please specify a valid VSD string.");
    }
    if (!Fs3UpdateSection(vsdStr, FS3_DEV_INFO, true, CMD_SET_VSD, printFunc)) {
        return false;
    }
    return true;
}

bool Fs3Operations::FwSetAccessKey(hw_key_t userKey, ProgressCallBack progressFunc)
{
    userKey = (hw_key_t){0,0};
    progressFunc = (ProgressCallBack)NULL;
    return errmsg("Set access key not supported.");
}
