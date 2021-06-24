/****************************************************************************
 *
 * (C) 2021 Cadence Design Systems, Inc. All rights reserved worldwide.
 *
 * This sample source code is not supported by Cadence Design Systems, Inc.
 * It is provided freely for demonstration purposes only.
 * SEE THE WARRANTY DISCLAIMER AT THE BOTTOM OF THIS FILE.
 *
 ***************************************************************************/
/****************************************************************************
 *
 * class CaeUnsUMCPSEG
 *
 ***************************************************************************/

#include "apiCAEP.h"
#include "apiCAEPUtils.h"
#include "apiGridModel.h"
#include "apiPWP.h"
#include "runtimeWrite.h"
#include "pwpPlatform.h"

#include "CaePlugin.h"
#include "CaeUnsGridModel.h"
#include "CaeUnsUMCPSEG.h"

#include<algorithm>
#include<cassert>
#include<string>


#if defined(DEBUG)
#   define DRVAL(dval,rval) (dval)
#else
#   define DRVAL(dval,rval) (rval)
#endif


const char *CreateLog   = "CreateLog";


static char
matIdChar(const MaterialId matId)
{
    static const char idMap[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    return (matId < 0 || matId > 35) ? '?' : idMap[matId];
}


template<typename T>
static const T&
makeInfo(const char *phystype, PWP_INT32 id)
{
    static T ret;
    ret.phystype = phystype;
    ret.id = id;
    return ret;
}


//***************************************************************************
//***************************************************************************
//***************************************************************************

BcInfoArray1    CaeUnsUMCPSEG::bcInfo_;
VcInfoArray1    CaeUnsUMCPSEG::vcInfo_;
StdStringCache  CaeUnsUMCPSEG::typeNames_;


CaeUnsUMCPSEG::CaeUnsUMCPSEG(CAEP_RTITEM *pRti, PWGM_HGRIDMODEL model,
        const CAEP_WRITEINFO *pWriteInfo) :
    CaeUnsPlugin(pRti, model, pWriteInfo),
    nodeInfo_(),
    geomEdges_(),
    log_(),
    curBlkId_(PWP_UINT32_UNDEF),
    curBlkCond_(),
    curDomId_(PWP_UINT32_UNDEF),
    curDomCond_()
{
}


CaeUnsUMCPSEG::~CaeUnsUMCPSEG()
{
}


bool
CaeUnsUMCPSEG::beginExport()
{
    setProgressMajorSteps(3);

    bool createLog = false;
    model_.getAttribute(CreateLog, createLog, createLog);
    if (createLog) {
        std::string logFile(writeInfo_.fileDest);
        logFile += ".log";
        log_.open(logFile, pwpWrite | pwpAscii);
    }

    if (log_.isOpen()) {
        log_.write("# To process node and edge data in this log file, source\n"
                   "# this log into a script that defines two procs that are\n"
                   "# compatable with the following signatures:\n");
        log_.write("\n");
        log_.write("# proc node { nodeId pt matId matConflict isBndry zoneId "
                                 "zoneConflict nborIds } {\n");
        log_.write("#   your NODE code here!\n");
        log_.write("# }\n");
        log_.write("\n");
        log_.write("# proc edge { ndx0 pt0 ndx1 pt1 } {\n");
        log_.write("#   your GEOM code here!\n");
        log_.write("# }\n");
        log_.write("\n");
        log_.write("# source {your.nlist.log}\n");
        log_.write("\n");

        log_.writef("set UndefinedMatId %d\n", int(MatUndefined));
        log_.writef("set UndefinedZoneId %d\n", int(ZoneUndefined));
        log_.write("\n");
    }
    return true;
}


PWP_BOOL
CaeUnsUMCPSEG::write()
{
    return init() && writeHeader() && writeNodes() && writeFaces() &&
        writeGeometry();
}


bool
CaeUnsUMCPSEG::endExport()
{
    return true;
}


bool
CaeUnsUMCPSEG::init()
{
    const PWGM_ENUM_FACEORDER order = PWGM_FACEORDER_DONTCARE;
    // Stream the faces (in this case, 2D edges) of the grid and identify the
    // material id and zone id of each node and classify each edge as boundary
    // or interior. See comments for streamFace() for more details.
    return model_.streamFaces(order, *this);
}


bool
CaeUnsUMCPSEG::writeOneNode(const CaeUnsVertex &v, const NodeInfo &ptInfo)
{
    // line 1
    //         1         2         3         4         5         6
    //123456789012345678901234567890123456789012345678901234567890
    // 6.85000000000000D-01 3.14500000000000D+00    5  0 0  0 1
    bool hadMatConflict = false;
    bool hadZoneConflict = false;
    const MaterialId matId = ptInfo.getMaterial(hadMatConflict);
    const ZoneId zoneId = ptInfo.getZone(hadZoneConflict);
    bool ret = rtFile_.writef("%21.14E%21.14E%5d %2d %c %2d%2d\n",
        double(v.x()), double(v.y()), int(ptInfo.nborCount()), int(matId),
        matIdChar(matId), int(ptInfo.isBndry() ? 1 : 0), int(zoneId));

    // line 2
    //         1         2         3         4         5
    //12345678901234567890123456789012345678901234567890
    //     59   5513     60   5538   5539   2262   2251
    const UInt32Array1& nbors = ptInfo.nbors();
    switch (nbors.size()) {
    case 1: // INVALID
        ret = false;
        assert(ret);
        break;
    case 2:
        ret = rtFile_.writef("%7d%7d\n", (int)(nbors.at(0) + 1),
            (int)(nbors.at(1) + 1));
        break;
    case 3:
        ret = rtFile_.writef("%7d%7d%7d\n", (int)(nbors.at(0) + 1),
            (int)(nbors.at(1) + 1), (int)(nbors.at(2) + 1));
        break;
    case 4:
        ret = rtFile_.writef("%7d%7d%7d%7d\n", (int)(nbors.at(0) + 1),
            (int)(nbors.at(1) + 1), (int)(nbors.at(2) + 1),
            (int)(nbors.at(3) + 1));
        break;
    case 5:
        ret = rtFile_.writef("%7d%7d%7d%7d%7d\n", (int)(nbors.at(0) + 1),
            (int)(nbors.at(1) + 1), (int)(nbors.at(2) + 1),
            (int)(nbors.at(3) + 1), (int)(nbors.at(4) + 1));
        break;
    case 6:
        ret = rtFile_.writef("%7d%7d%7d%7d%7d%7d\n", (int)(nbors.at(0) + 1),
            (int)(nbors.at(1) + 1), (int)(nbors.at(2) + 1),
            (int)(nbors.at(3) + 1), (int)(nbors.at(4) + 1),
            (int)(nbors.at(5) + 1));
        break;
    case 7:
        ret = rtFile_.writef("%7d%7d%7d%7d%7d%7d%7d\n", (int)(nbors.at(0) + 1),
            (int)(nbors.at(1) + 1), (int)(nbors.at(2) + 1),
            (int)(nbors.at(3) + 1), (int)(nbors.at(4) + 1),
            (int)(nbors.at(5) + 1), (int)(nbors.at(6) + 1));
        break;
    default: {
        // > 7 neighbors, use slower loop!
        UInt32Array1::const_iterator nit = ptInfo.nbors().begin();
        for (; ret && ptInfo.nbors().end() != nit; ++nit) {
            ret = rtFile_.writef("%7d", (int)((*nit) + 1));
        }
        ret = ret && rtFile_.write("\n");
        break; }
    }

    if (ret && log_.isOpen()) {
        log_.writef("node %d {%g %g %g} %d %d %d %d %d {",
            int(v.index() + 1), double(v.x()), double(v.y()), double(v.z()),
            int(matId), int(hadMatConflict), int(ptInfo.isBndry() ? 1 : 0),
            int(zoneId), int(hadZoneConflict));
        UInt32Array1::const_iterator nit = ptInfo.nbors().begin();
        log_.write((*nit) + 1);   // first neighbor
        for (++nit; ptInfo.nbors().end() != nit; ++nit) {
            log_.write((*nit) + 1, 0, " ");   // next neighbor
        }
        log_.write("}\n");
    }

    return ret;
}


bool
CaeUnsUMCPSEG::writeHeader()
{
    char strTime[256];
    time_t szClock;
    time(&szClock);
    strftime(strTime, sizeof(strTime), "%Y-%m-%d %H:%M:%S", localtime(&szClock));

    const char *appMach = 0;
    model_.getAttribute("AppMachine", appMach, "Unknown");

    const char *appVer;
    model_.getAttribute("AppNameAndVersion", appVer, "Pointwise");
    return rtFile_.write("POINTWISE\n") &&
        rtFile_.writef("Created by %s on %s (%s)\n", appVer, strTime, appMach);
}


bool
CaeUnsUMCPSEG::writeNodes()
{
    //         1         2         3         4
    //1234567890123456789012345678901234567890
    //   7468     5          ***** NODES *****
    const PWP_UINT32 subType = 5; // indicates a POINTWISE generated file
    // changed subType to 5 in order for CPSEG to denote read changes
    rtFile_.writef("%7d %5d          ***** NODES *****\n",
        (int)model_.vertexCount(), (int)subType);

    bool ret = progressBeginStep(model_.vertexCount());
    if (ret) {
        CaeUnsVertex v(model_);
        while (v.isValid()) {
            NInfoMap::const_iterator it = nodeInfo_.find(v.index());
            if (nodeInfo_.end() == it) {
                sendErrorMsg("Could not find neighbor points");
                ret = false;
                break;
            }
            writeOneNode(v, it->second);
            ++v;
            if (!progressIncrement()) {
                ret = false;
                break;
            }
        }
    }
    progressEndStep();
    return ret;
}


bool
CaeUnsUMCPSEG::writeOneFace(const PWP_UINT32 n0, const PWP_UINT32 n1,
    const PWP_UINT32 n2)
{
    //         1         2         3         4
    //1234567890123456789012345678901234567890
    //   6159   6173   6160   6160
    // ...snip...
    //   6175   6109   6174   6174
    //
    // yes, n2 is repeated (collapsed quad?)
    return rtFile_.writef("%7d%7d%7d%7d\n", (int)(n0 + 1), (int)(n1 + 1),
        (int)(n2 + 1), (int)(n2 + 1)) && progressIncrement();
}


bool
CaeUnsUMCPSEG::writeFaces()
{
    //         1         2         3         4
    //1234567890123456789012345678901234567890
    //  14757          ***** FACES *****
    PWGM_ELEMCOUNTS cnts;
    model_.elementCount(&cnts);
    const int faceCnt = (int)(PWGM_ECNT_Quad(cnts) * 2 + PWGM_ECNT_Tri(cnts));
    rtFile_.writef("%7d        ***** FACES *****\n", faceCnt);

    bool ret = progressBeginStep(model_.elementCount());
    PWGM_ELEMDATA d;
    CaeUnsElement e(model_);
    while (ret && e.isValid()) {
        if (!e.data(d)) {
            ret = false;
        }
        else if (PWGM_ELEMTYPE_TRI == d.type) {
            ret = writeOneFace(d.index[0], d.index[1], d.index[2]);
        }
        else if (PWGM_ELEMTYPE_QUAD == d.type) {
            // write quads as two tris
            ret = writeOneFace(d.index[0], d.index[1], d.index[2]) &&
                writeOneFace(d.index[0], d.index[2], d.index[3]);
        }
        else {
            sendErrorMsg("writeFaces: Unexpected element type");
            ret = false;
        }
        ++e;
    }
    progressEndStep();
    return ret;
}


bool
CaeUnsUMCPSEG::writeGeometry()
{
    //         1         2         3         4
    //1234567890123456789012345678901234567890
    //    906          ***** GEOMETRY *****
    rtFile_.writef("%7d          ***** GEOMETRY *****\n",
        (int)geomEdges_.size());

    bool ret = progressBeginStep(PWP_UINT32(geomEdges_.size()));
    if (ret) {
        //         1         2         3         4
        //1234567890123456789012345678901234567890
        //  0.00000E+00  0.00000E+00  2.02000E-01  0.00000E+00
        // ...snip...
        //  2.02000E-01  0.00000E+00  4.04000E-01  0.00000E+00
        EdgeArray1::const_iterator it;
        for (it = geomEdges_.begin(); geomEdges_.end() != it; ++it) {
            const CaeUnsVertex v0(model_, it->first);
            const CaeUnsVertex v1(model_, it->second);
            rtFile_.writef("%13.5E%13.5E%13.5E%13.5E\n", double(v0.x()),
                double(v0.y()), double(v1.x()), double(v1.y()));
            if (log_.isOpen()) {
                log_.writef("edge %d {%g %g %g} %d {%g %g %g}\n",
                    int(it->first), double(v0.x()), double(v0.y()),
                    double(v0.z()), int(it->second), double(v1.x()),
                    double(v1.y()), double(v1.z()));
            }
            if (!progressIncrement()) {
                ret = false;
                break;
            }
        }
    }
    progressEndStep();
    return ret;
}


//===========================================================================
// face streaming handlers
//===========================================================================

PWP_UINT32
CaeUnsUMCPSEG::streamBegin(const PWGM_BEGINSTREAM_DATA &data)
{
    // This is a rough guess
    geomEdges_.reserve(data.numBoundaryFaces * 2);
    return 1;
}


/* Each node has a VC-assigned material and zone id, a BC-assigned material and
   zone id, and a list of neighbor nodes. Initially these values are all
   undefined/empty. See class NodeInfo.

   Examine the edge's BC and its owner/neighbor VCs and push the appropriate
   material id and zone id values to each of the edge's nodes. The data is
   pushed to each node with the other node as its neighbor. A higher zone id
   value will overwrite a lower zone id value.

   Once all edges have been processed, each node will have a final material and
   zone id. The BC assigned material and zone id values will have precedence
   over the VC assigned material and zone id values.
*/
PWP_UINT32
CaeUnsUMCPSEG::streamFace(const PWGM_FACESTREAM_DATA &data)
{
    // Since this is a 2D exporter, data defines an edge (PWGM_ELEMTYPE_BAR).
    PWP_UINT32 ret = 0;
    if (PWGM_ELEMTYPE_BAR == data.elemData.type) {
        const bool isBndry = (PWGM_FACETYPE_BOUNDARY == data.type);
        const PWP_UINT32 &ndx0 = data.elemData.index[0];
        const PWP_UINT32 &ndx1 = data.elemData.index[1];
        MaterialId matId = 0;
        ZoneId zoneId = UndefinedId;
        bool mzFromVC = false; // true if matId and zoneId came from a VC
        bool isGeomEdge = false; // true if edge should be added to geomEdges_
        if (getEdgeMatAndZone(data, matId, zoneId, mzFromVC, isGeomEdge) &&
                pushPt(ndx0, ndx1, matId, zoneId, mzFromVC, isBndry) &&
                pushPt(ndx1, ndx0, matId, zoneId, mzFromVC, isBndry)) {
            if (isGeomEdge) {
                geomEdges_.push_back(Edge(ndx0, ndx1));
            }
            ret = 1;
        }
    }
    return ret;
}


PWP_UINT32
CaeUnsUMCPSEG::streamEnd(const PWGM_ENDSTREAM_DATA & /*data*/)
{
    return 1;
}


bool
CaeUnsUMCPSEG::getPoint(const PWP_UINT32 vPt, NInfoIter &it,
    const bool allowCreate)
{
    it = nodeInfo_.find(vPt);
    if (allowCreate && (nodeInfo_.end() == it)) {
        // vPt is new - create default map entry
        it = nodeInfo_.insert(NInfoMapVal(vPt, NodeInfo())).first;
    }
    return nodeInfo_.end() != it;
}


NInfoIter
CaeUnsUMCPSEG::addPoint(const PWP_UINT32 vPt)
{
    NInfoIter it;
    getPoint(vPt, it, true);
    assert(it != nodeInfo_.end());
    return it;
}


bool
CaeUnsUMCPSEG::pushPt(const PWP_UINT32 vPt, const PWP_UINT32 vNbor,
    const MaterialId matId, const ZoneId zoneId, const bool mzFromVC,
    const bool isBndry)
{
    NInfoIter it;
    const bool ret = getPoint(vPt, it, true);
    if (ret) {
        NodeInfo &ni = it->second;
        ni.nbors().push_back(vNbor);
        if (isBndry) {
            ni.setBndry();
        }
        if (mzFromVC) {
            ni.setVCMaterial(matId);
            ni.setVCZone(zoneId);
        }
        else {
            ni.setBCMaterial(matId);
            ni.setBCZone(zoneId);
        }
    }
    return ret;
}


bool
CaeUnsUMCPSEG::createBCsAndVCs(CAEP_RTITEM &rti)
{
    if (!bcInfo_.empty()) {
        // We have already created all the BC avd VC data
        return true;
    }

    const PWP_INT32 MaterialCnt = 36; // getEnvMaterialCount();

    // Preload static BCs from rtCaepSupportData.h
    bcInfo_.clear();
    bcInfo_.reserve(MaterialCnt + rti.BCCnt);
    for (PWP_INT32 ii = 0; ii < PWP_INT32(rti.BCCnt); ++ii) {
        const CAEP_BCINFO &info = rti.pBCInfo[ii];
        bcInfo_.push_back(makeInfo<CAEP_BCINFO>(info.phystype, info.id));
    }

    // Preload static VCs from rtCaepSupportData.h
    vcInfo_.clear();
    bcInfo_.reserve(MaterialCnt + rti.VCCnt);
    for (PWP_INT32 ii = 0; ii < PWP_INT32(rti.VCCnt); ++ii) {
        const CAEP_VCINFO &info = rti.pVCInfo[ii];
        vcInfo_.push_back(makeInfo<CAEP_VCINFO>(info.phystype, info.id));
    }

    // Init with the non-inflated static BC phystypes
    std::string shadowTypes(""); // e.g. "type1|type2|type3"

    // Generate dynamically generated materials
    for (PWP_INT32 id = 0; id < MaterialCnt; ++id) {
        char phystype[128];
        sprintf(phystype, "Material-%c", matIdChar(MaterialId(id)));
        // cache phystype so ptr used below is persistent
        typeNames_.push_back(phystype);
        const char *p = typeNames_.back().c_str();
        bcInfo_.push_back(makeInfo<CAEP_BCINFO>(p, id + 1));
        vcInfo_.push_back(makeInfo<CAEP_VCINFO>(p, id + 1));
        // all material types are non-inflatable
        if (!shadowTypes.empty()) {
            shadowTypes += "|";
        }
        shadowTypes += phystype;
    }

    // Replace statically declared BCs and VCs with bcInfo_ and vcInfo_
    rti.BCCnt = PWP_UINT32(bcInfo_.size());
    rti.pBCInfo = bcInfo_.data();
    rti.VCCnt = PWP_UINT32(vcInfo_.size());
    rti.pVCInfo = vcInfo_.data();

    bool ret = true;

    ret = ret &&
        caeuAssignInfoValue("ShadowBcTypes", shadowTypes.c_str(), true);

    return ret;
}


bool
CaeUnsUMCPSEG::getEdgeMatAndZone(const PWGM_FACESTREAM_DATA &data,
    MaterialId &matId, ZoneId &zoneId, bool &mzFromVC, bool &isGeomEdge) const
{
    bool ret;
    switch (data.type) {
    case PWGM_FACETYPE_BOUNDARY:
        ret = getBndryEdgeMatAndZone(data, matId, zoneId, mzFromVC, isGeomEdge);
        break;
    case PWGM_FACETYPE_INTERIOR:
        ret = getIntorEdgeMatAndZone(data, matId, zoneId, mzFromVC, isGeomEdge);
        break;
    case PWGM_FACETYPE_CONNECTION:
        ret = getCnxnEdgeMatAndZone(data, matId, zoneId, mzFromVC, isGeomEdge);
        break;
    default:
        matId = MatUndefined;
        zoneId = ZoneUndefined;
        ret = false;
        break;
    }
    return ret;
}


bool
CaeUnsUMCPSEG::getBndryEdgeMatAndZone(const PWGM_FACESTREAM_DATA &data,
    MaterialId &matId, ZoneId &zoneId, bool &mzFromVC, bool &isGeomEdge) const
{
    bool useVC = true;
    if (PWGM_HDOMAIN_ISVALID(data.owner.domain)) {
        mzFromVC = false;
        getMatAndZone(data.owner.domain, matId, zoneId);
        useVC = (MatUndefined == matId || ZoneUndefined == zoneId);
    }

    if (useVC) {
        mzFromVC = true;
        getMatAndZone(data.owner.block, matId, zoneId);
    }

    isGeomEdge = true; // always
    return true;
}


bool
CaeUnsUMCPSEG::getIntorEdgeMatAndZone(const PWGM_FACESTREAM_DATA &data,
    MaterialId &matId, ZoneId &zoneId, bool &mzFromVC, bool &isGeomEdge) const
{
    bool ret = false;
    const CaeUnsBlock owner(data.owner.block);
    CaeUnsBlock nbor;
    if (!getNborBlk(data, nbor)) {
        isGeomEdge = false;
        mzFromVC = false;
        matId = MatUndefined;
        zoneId = ZoneUndefined;
    }
    else if (owner.index() == nbor.index()) {
        // Same block on both sides. No need to get the neighbor info.
        getMatAndZone(data.owner.block, matId, zoneId);
        mzFromVC = true;
        isGeomEdge = false;
        ret = true;
    }
    else {
        // edge is between different blocks
        getMatAndZone(data.owner.block, matId, zoneId);
        MaterialId nborMatId;
        ZoneId nborZoneId;
        getMatAndZone(nbor, nborMatId, nborZoneId);
        isGeomEdge = (nborMatId != matId) || (nborZoneId != zoneId);
        if (isGeomEdge) {
            // edge is between elements with different material and/or zone ids
            if (nborMatId == matId) {
                // The materials are the same. The zones must be different.
                // Capture the max of the zone ids.
                zoneId = std::max(nborZoneId, zoneId);
            }
            else if (nborMatId > matId) {
                // Capture larger nborMatId and its corresponding nborZoneId
                matId = nborMatId;
                zoneId = nborZoneId;
            }
            // else nborMatId < matId
            //  matId and zoneId are good to go
        }
        mzFromVC = true;
        ret = true;
    }
    return ret;
}


bool
CaeUnsUMCPSEG::getCnxnEdgeMatAndZone(const PWGM_FACESTREAM_DATA &data,
    MaterialId &matId, ZoneId &zoneId, bool &mzFromVC, bool &isGeomEdge) const
{
    // a connection is the same as interior with one or both of:
    //  * different VCs on either side
    //  * member of a non-inflated BC
    // So, first get interior status. If also member of a BC, do more
    bool ret = getIntorEdgeMatAndZone(data, matId, zoneId, mzFromVC,
                isGeomEdge);
    const PWGM_HDOMAIN &h = data.owner.domain;
    if (PWGM_HDOMAIN_ISVALID(h)) {
        // edge is also a member of a non-inflated BC
        MaterialId eMatId;
        ZoneId eZoneId;
        getMatAndZone(h, eMatId, eZoneId);
        isGeomEdge = isGeomEdge || (eMatId != matId) || (eZoneId != zoneId);
        // BC mat/zone always take precedence over VC mat/zone
        matId = eMatId;
        zoneId = eZoneId;
        mzFromVC = false;
    }
    return ret;
}


bool
CaeUnsUMCPSEG::getNborBlk(const PWGM_FACESTREAM_DATA &d, CaeUnsBlock &blk) const
{
    PWGM_ENUMELEMDATA eed;
    const PWGM_HELEMENT hE = PwModEnumElements(d.model, d.neighborCellIndex);
    return PwElemDataModEnum(hE, &eed)
        ? blk.moveTo(model_, PWGM_HELEMENT_PID(eed.hBlkElement)).isValid()
        : false;
}


void
CaeUnsUMCPSEG::getMatAndZone(const PWGM_CONDDATA &cd, MaterialId &matId,
    ZoneId &zoneId)
{
    if (0 == cd.tid) {
        matId = MatUndefined;
        zoneId = ZoneUndefined;
    }
    else {
        matId = static_cast<MaterialId>(cd.tid - 1);
        zoneId = static_cast<ZoneId>(cd.id);
    }
}


void
CaeUnsUMCPSEG::getMatAndZone(const PWGM_HBLOCK &h, MaterialId &matId,
    ZoneId &zoneId) const
{
    if (PWGM_HBLOCK_ID(h) != curBlkId_) {
        if (CaeUnsBlock(h).condition(curBlkCond_)) {
            curBlkId_ = PWGM_HBLOCK_ID(h);
        }
        else {
            curBlkId_ = PWP_UINT32_UNDEF;
        }
    }
    if (PWP_UINT32_UNDEF == curBlkId_) {
        matId = MatUndefined;
        zoneId = ZoneUndefined;
    }
    else {
        getMatAndZone(curBlkCond_, matId, zoneId);
    }
}


void
CaeUnsUMCPSEG::getMatAndZone(const PWGM_HDOMAIN &h, MaterialId &matId,
    ZoneId &zoneId) const
{
    if (PWGM_HDOMAIN_ID(h) != curDomId_) {
        if (CaeUnsPatch(h).condition(curDomCond_)) {
            curDomId_ = PWGM_HDOMAIN_ID(h);
        }
        else {
            curDomId_ = PWP_UINT32_UNDEF;
        }
    }
    if (PWP_UINT32_UNDEF == curDomId_) {
        matId = MatUndefined;
        zoneId = ZoneUndefined;
    }
    else {
        getMatAndZone(curDomCond_, matId, zoneId);
    }
}


//===========================================================================
// called ONCE when plugin first loaded into memory
//===========================================================================

bool
CaeUnsUMCPSEG::create(CAEP_RTITEM &rti)
{
    (void)rti.BCCnt; // silence unused arg warning
    bool ret = createBCsAndVCs(rti);

    ret = ret && publishBoolValueDef(rti, CreateLog, DRVAL(true, false),
            "Controls generation of a log file for debugging.");

    return ret;
}


//===========================================================================
// called ONCE just before plugin unloaded from memory
//===========================================================================

void
CaeUnsUMCPSEG::destroy(CAEP_RTITEM &rti)
{
    (void)rti.BCCnt; // silence unused arg warning
}


/****************************************************************************
 *
 * This file is licensed under the Cadence Public License Version 1.0 (the
 * "License"), a copy of which is found in the included file named "LICENSE",
 * and is distributed "AS IS." TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE
 * LAW, CADENCE DISCLAIMS ALL WARRANTIES AND IN NO EVENT SHALL BE LIABLE TO
 * ANY PARTY FOR ANY DAMAGES ARISING OUT OF OR RELATING TO USE OF THIS FILE.
 * Please see the License for the full text of applicable terms.
 *
 ****************************************************************************/
