/****************************************************************************
 *
 * class CaeUnsUMCPSEG
 *
 * Copyright (c) 2012-2018 Pointwise, Inc.
 * All rights reserved.
 *
 * This sample Pointwise plugin is not supported by Pointwise, Inc.
 * It is provided freely for demonstration purposes only.
 * SEE THE WARRANTY DISCLAIMER AT THE BOTTOM OF THIS FILE.
 *
 ***************************************************************************/

#ifndef _CAEUNSUMICH_H_
#define _CAEUNSUMICH_H_

#include "apiGridModel.h"
#include "apiPWP.h"

#include "CaePlugin.h"
#include "CaeUnsGridModel.h"

#include<cassert>
#include<list>
#include<map>
#include<utility>
#include<vector>


typedef std::vector<PWP_UINT32>             UInt32Array1; 
typedef std::vector<CAEP_BCINFO>            BcInfoArray1; 
typedef std::vector<CAEP_VCINFO>            VcInfoArray1; 
typedef std::list<std::string>              StdStringCache; 
typedef std::pair<PWP_UINT32, PWP_UINT32>   Edge;
typedef std::vector<Edge>                   EdgeArray1; 
typedef PWP_INT32                           IdType;
typedef IdType                              MaterialId;
typedef IdType                              ZoneId;


const IdType        UndefinedId = -1;
const MaterialId    MatUndefined = UndefinedId;
const ZoneId        ZoneUndefined = UndefinedId;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class ManagedId {
public:
    ManagedId(const bool preferPositiveIds = false) :
        preferPositiveIds_(preferPositiveIds),
        id_(UndefinedId),
        hadConflict_(false)
    {
    }

    ~ManagedId()
    {
    }

    void setId(const IdType id)
    {
        // In general, this objects captures the lowest id value that is >= 0.
        // However, if preferPositiveIds_ is true, we want the lowest positive
        // value to always replace a zero.
        if (UndefinedId == id) {
            // incoming id is undefined - skip it
        }
        else if (UndefinedId == id_) {
            id_ = id; // capture first id even if zero
        }
        else if (preferPositiveIds_ && (0 == id)) {
            // incoming id is zero - skip it
        }
        else if (preferPositiveIds_ && (0 == id_)) {
            // A zero was captured as first value above. The incoming id is > 0.
            // So, id superceeds the cached zero. The only way id_ will stay
            // zero is if all calls pass in zeros (which is valid).
            id_ = id; // capture first positive, nonzero id
            hadConflict_ = true;
        }
        else if (id > id_) {
            id_ = id; // cature the lowest id
            hadConflict_ = true;
        }
// >JK: 1/2018 Changing the logic here to have the highest MatId have the ownership
        else if (id < id_) {
            hadConflict_ = true;
        }
        // else id == id_ // a NOP
    }

    IdType getId() const
    {
        return id_;
    }

    bool isSet(IdType &id, bool &hadConflict) const
    {
        hadConflict = hadConflict_;
        return UndefinedId != (id = id_);
    }

private:

    bool    preferPositiveIds_;
    IdType  id_;
    bool    hadConflict_;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class NodeInfo {
public:

    NodeInfo() :
        isBndry_(false),
        elemMaterial_(true),
        edgeMaterial_(true),
        elemZone_(),
        edgeZone_(),
        nbors_()
    {
    }

    ~NodeInfo()
    {
    }

    UInt32Array1&       nbors() {
                            return nbors_; }

    const UInt32Array1& nbors() const {
                            return nbors_; }

    PWP_UINT32          nborCount() const {
                            return PWP_UINT32(nbors_.size()); }

    MaterialId  getMaterial(bool &hadConflict) const {
                    return evalIds(elemMaterial_, edgeMaterial_, hadConflict); }

    void        setVCMaterial(const MaterialId id) {
                    elemMaterial_.setId(id); }

    void        setBCMaterial(const MaterialId id) {
                    edgeMaterial_.setId(id); }

    ZoneId      getZone(bool &hadConflict) const {
                    return evalIds(elemZone_, edgeZone_, hadConflict); }

    void        setVCZone(const MaterialId id) {
                    elemZone_.setId(id); }

    void        setBCZone(const MaterialId id) {
                    edgeZone_.setId(id); }

    bool        isBndry() const {
                    return isBndry_; }

    void        setBndry() {
                    isBndry_ = true; }


private:

    IdType evalIds(const ManagedId &elem, const ManagedId &edge,
        bool &hadConf) const
    {
        IdType ret;
        if (edge.isSet(ret, hadConf)) {
            // edge Ids have precedence
        }
        else if (elem.isSet(ret, hadConf)) {
            // otherwise use id from elem
        }
        else {
            // Will get here if a point is NOT part of a BC or VC
            //assert(!int("evalIds: Undefined id value"));
            ret = UndefinedId;
            hadConf = true;
        }
        return ret;
    }


private:

    bool            isBndry_;
    ManagedId       elemMaterial_;
    ManagedId       edgeMaterial_;
    ManagedId       elemZone_;
    ManagedId       edgeZone_;
    UInt32Array1    nbors_;
};



typedef std::map<PWP_UINT32, NodeInfo>  NInfoMap;
typedef NInfoMap::value_type            NInfoMapVal;
typedef NInfoMap::iterator              NInfoIter;
typedef NInfoMap::const_iterator        NInfoCIter;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class CaeUnsUMCPSEG : public CaeUnsPlugin, public CaeFaceStreamHandler {
public:
    CaeUnsUMCPSEG(CAEP_RTITEM *pRti, PWGM_HGRIDMODEL model,
        const CAEP_WRITEINFO *pWriteInfo);

    virtual ~CaeUnsUMCPSEG();

    static bool create(CAEP_RTITEM &rti);

    static void destroy(CAEP_RTITEM &rti);

private:
    virtual bool        beginExport();
    virtual PWP_BOOL    write();
    virtual bool        endExport();

    // face streaming handlers
    virtual PWP_UINT32 streamBegin(const PWGM_BEGINSTREAM_DATA &data);
    virtual PWP_UINT32 streamFace(const PWGM_FACESTREAM_DATA &data);
    virtual PWP_UINT32 streamEnd(const PWGM_ENDSTREAM_DATA &data);

private:
    // Plugin implementation helper methods

    bool        init();
    bool        writeHeader();
    bool        writeNodes();
    bool        writeOneNode(const CaeUnsVertex &v, const NodeInfo &info);
    bool        writeFaces();
    bool        writeOneFace(const PWP_UINT32 n0, const PWP_UINT32 n1,
                    const PWP_UINT32 n2);
    bool        writeGeometry();

    bool        getPoint(const PWP_UINT32 vPt, NInfoIter &it,
                    const bool allowCreate = false);

    NInfoIter   addPoint(const PWP_UINT32 vPt);

    bool        pushPt(const PWP_UINT32 vPt, const PWP_UINT32 vNbor,
                    const MaterialId matId, const ZoneId zoneId,
                    const bool mzFromVC, const bool isBndry);

    static bool createBCsAndVCs(CAEP_RTITEM &rti);

    bool    getEdgeMatAndZone(const PWGM_FACESTREAM_DATA &data,
                MaterialId &matId, ZoneId &zoneId, bool &mzFromVC,
                bool &isGeomEdge) const;

    bool    getBndryEdgeMatAndZone(const PWGM_FACESTREAM_DATA &data,
                MaterialId &matId, ZoneId &zoneId, bool &mzFromVC,
                bool &isGeomEdge) const;

    bool    getIntorEdgeMatAndZone(const PWGM_FACESTREAM_DATA &data,
                MaterialId &matId, ZoneId &zoneId, bool &mzFromVC,
                bool &isGeomEdge) const;

    bool    getCnxnEdgeMatAndZone(const PWGM_FACESTREAM_DATA &data,
                MaterialId &matId, ZoneId &zoneId, bool &mzFromVC,
                bool &isGeomEdge) const;

    bool    getNborBlk(const PWGM_FACESTREAM_DATA &d, CaeUnsBlock &hBlk) const;

    void    getMatAndZone(const PWGM_HBLOCK &h, MaterialId &matId,
                ZoneId &zoneId) const;

    void    getMatAndZone(const PWGM_HDOMAIN &h, MaterialId &matId,
                ZoneId &zoneId) const;

    static void getMatAndZone(const PWGM_CONDDATA &cd, MaterialId &matId,
                    ZoneId &zoneId);


private:
    // Maps a vertex index to its accociated NodeInfo object
    NInfoMap                nodeInfo_;

    // Array of Edge objects
    EdgeArray1              geomEdges_;

    // Debug log file (dis/enabled by "CreateLog" solver attribute)
    PwpFile                 log_;

    // Id of current block being processed (transient runtime value)
    mutable PWP_UINT32      curBlkId_;

    // VC of current block being processed (transient runtime value)
    mutable PWGM_CONDDATA   curBlkCond_;

    // Id of current domain being processed (transient runtime value)
    mutable PWP_UINT32      curDomId_;

    // BC of current domain being processed (transient runtime value)
    mutable PWGM_CONDDATA   curDomCond_;

    // Collection of BC and VC type names generated by createBCsAndVCs().
    static StdStringCache   typeNames_;

    // Collection of CAEP_BCINFO objects generated by createBCsAndVCs().
    static BcInfoArray1     bcInfo_;

    // Collection of CAEP_VCINFO objects generated by createBCsAndVCs().
    static VcInfoArray1     vcInfo_;
};

#endif // _CAEUNSUMICH_H_


/****************************************************************************
 *
 * DISCLAIMER:
 * TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, POINTWISE DISCLAIMS
 * ALL WARRANTIES, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, WITH REGARD TO THIS SCRIPT. TO THE MAXIMUM EXTENT PERMITTED
 * BY APPLICABLE LAW, IN NO EVENT SHALL POINTWISE BE LIABLE TO ANY PARTY
 * FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
 * WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF
 * BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE
 * USE OF OR INABILITY TO USE THIS SCRIPT EVEN IF POINTWISE HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGES AND REGARDLESS OF THE
 * FAULT OR NEGLIGENCE OF POINTWISE.
 *
 ***************************************************************************/
