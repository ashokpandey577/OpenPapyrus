// BHT.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

#define FNAMELEN   12
#define NUMRECSLEN  5
#define RECNOLEN    5
#define NUMFLDSLEN  2
#define FLDLENLEN   2
#define MAXBHTRECS  32767
//
//
//
SLAPI StyloBhtIIOnHostCfg::StyloBhtIIOnHostCfg() : P_OpList(0)
{
	Init();
}

SLAPI StyloBhtIIOnHostCfg::~StyloBhtIIOnHostCfg()
{
	ZDELETE(P_OpList);
}

void SLAPI StyloBhtIIOnHostCfg::Init()
{
	DeviceName.Z();
	WeightPrefix.Z();
	QttyWeightPrefix.Z();
	UserName.Z();
	Password.Z();
	ServerAddr   = 0;
	ServerPort   = 0;
	ServerMask   = 0;
	Flags        = 0;
	BcdPrinterID = 0;
	DefQtty      = 0;
	ExportBillsPeriod.SetZero();
	ZDELETE(P_OpList);
}

StyloBhtIIOnHostCfg & FASTCALL StyloBhtIIOnHostCfg::operator = (const StyloBhtIIOnHostCfg & rSrc)
{
	Init();
	DeviceName       = rSrc.DeviceName;
	WeightPrefix     = rSrc.WeightPrefix;
	QttyWeightPrefix = rSrc.QttyWeightPrefix;
	UserName         = rSrc.UserName;
	Password         = rSrc.Password;
	ServerAddr       = rSrc.ServerAddr;
	ServerPort       = rSrc.ServerPort;
	ServerMask       = rSrc.ServerMask;
	Flags            = rSrc.Flags;
	BcdPrinterID     = rSrc.BcdPrinterID;
	DefQtty          = rSrc.DefQtty;
	ExportBillsPeriod = rSrc.ExportBillsPeriod;
	if(rSrc.P_OpList) {
		P_OpList = new SBIIOpInfoArray;
		CALLPTRMEMB(P_OpList, copy(*rSrc.P_OpList));
	}
	return *this;
}

int SLAPI StyloBhtIIOnHostCfg::IsValid()
{
	int    ok = 1;
	THROW_PP(DeviceName.Len(), PPERR_NAMENEEDED);
	if(Flags & StyloBhtIIConfig::fUseWiFi) {
		THROW_PP(UserName.Len(), PPERR_INVUSERORPASSW);
		THROW_PP(Password.Len(), PPERR_INVUSERORPASSW);
		THROW_PP(ServerAddr, PPERR_INVIP);
	}
	THROW_PP(P_OpList && P_OpList->getCount(), PPERR_INVOP);
	for(uint i = 0; i < P_OpList->getCount(); i++) {
		const SBIIOpInfo & r_op_info = P_OpList->at(i);
		if(r_op_info.ToHostOpID == 0) {
			if(r_op_info.OpID > 0) {
				// @v9.4.9 PPObject::SetLastErrObj(PPOBJ_OPRKIND, r_op_info.OpID);
				// @v9.4.9 PPSetError(PPERR_INVTOHOSTOP);
				PPSetObjError(PPERR_INVTOHOSTOP, PPOBJ_OPRKIND, r_op_info.OpID); // @v9.4.9
			}
			else if(r_op_info.OpID == -StyloBhtIIConfig::oprkExpend)
				PPSetError(PPERR_INVEXPENDOPTOHOSTOP);
			else if(r_op_info.OpID == -StyloBhtIIConfig::oprkReceipt)
				PPSetError(PPERR_INVRECEIPTOPTOHOSTOP);
			else if(r_op_info.OpID == -StyloBhtIIConfig::oprkTransfer)
				PPSetError(PPERR_INVTRFROPTOHOSTOP);
			CALLEXCEPT();
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI StyloBhtIIOnHostCfg::IsEmpty() const
{
	return BIN(!DeviceName.Len() && !WeightPrefix.Len() && !QttyWeightPrefix.Len() &&
		!BcdPrinterID && !UserName.Len() && !Password.Len() && !ServerAddr && !ServerPort &&
		!ServerMask && !Flags && ExportBillsPeriod.IsZero() && !P_OpList);
}

PPID SLAPI StyloBhtIIOnHostCfg::GetOpID(PPID opID) const
{
	uint   pos = 0;
	return (P_OpList && P_OpList->lsearch(&opID, &pos, PTR_CMPFUNC(long)) > 0) ? P_OpList->at(pos).ToHostOpID : 0;
}

int SLAPI StyloBhtIIOnHostCfg::IsCostAsPrice(PPID opID) const
{
	uint   pos = 0;
	return (P_OpList && P_OpList->lsearch(&opID, &pos, PTR_CMPFUNC(long))) ? BIN(P_OpList->at(pos).Flags & StyloBhtIIConfig::foprCostAsPrice) : 0;
}
//
//
//
SLAPI PPBhtTerminalPacket::PPBhtTerminalPacket() : P_Filt(0), P_SBIICfg(0)
{
	MEMSZERO(Rec);
}

SLAPI PPBhtTerminalPacket::~PPBhtTerminalPacket()
{
	ZDELETE(P_Filt);
	ZDELETE(P_SBIICfg);
}

PPBhtTerminalPacket & FASTCALL PPBhtTerminalPacket::operator = (const PPBhtTerminalPacket & s)
{
	Rec = s.Rec;
	ZDELETE(P_Filt);
	ZDELETE(P_SBIICfg);
	if(s.P_Filt)
		P_Filt = new GoodsFilt(*s.P_Filt);
	if(s.P_SBIICfg) {
		P_SBIICfg = new StyloBhtIIOnHostCfg;
		*P_SBIICfg = *s.P_SBIICfg;
	}
	ImpExpPath_ = s.ImpExpPath_;
	return *this;
}

SLAPI PPObjBHT::PPObjBHT(void * extraPtr) : PPObjReference(PPOBJ_BHT, extraPtr), P_BObj(BillObj)
{
}

class BhtDialog : public TDialog {
public:
	BhtDialog() : TDialog(DLG_BHT)
	{
		FileBrowseCtrlGroup::Setup(this, CTLBRW_BHT_IMPEXPPATH, CTL_BHT_IMPEXPPATH, 1, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
	}
	int    setDTS(const PPBhtTerminalPacket *);
	int    getDTS(PPBhtTerminalPacket *);
private:
	DECL_HANDLE_EVENT;
	void   DisableCtrls();
	void   editGoodsFilt();
	void   editMore();

	PPBhtTerminalPacket Data;
};

IMPL_HANDLE_EVENT(BhtDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmBhtGoodsFilt))
		editGoodsFilt();
	else if(event.isCmd(cmaMore))
		editMore();
	else if(event.isClusterClk(CTL_BHT_TYPE))
		DisableCtrls();
	else
		return;
	clearEvent(event);
}

void BhtDialog::DisableCtrls()
{
	long   v = 0;
	int    disable_path = 0, disable_coms = 0, is_wince = 0;
	GetClusterData(CTL_BHT_TYPE, &v);
	getCtrlData(CTL_BHT_TYPE, &v);
	disable_coms = oneof4(v, PPObjBHT::btWinCe, PPObjBHT::btPalm, PPObjBHT::btCom, PPObjBHT::btStyloBhtII) ? 1 : 0;
	disable_path = oneof3(v, PPObjBHT::btWinCe, PPObjBHT::btPalm, PPObjBHT::btStyloBhtII) ? 0 : 1;
	is_wince = (v == PPObjBHT::btWinCe) ? 1 : 0;
	DisableClusterItem(CTL_BHT_FLAGS, 0, !is_wince);
	if(!is_wince) {
		GetClusterData(CTL_BHT_FLAGS, &Data.Rec.Flags);
		Data.Rec.Flags &= ~PPBhtTerminal::fDelAfterImport;
		SetClusterData(CTL_BHT_FLAGS, Data.Rec.Flags);
	}
	disableCtrls(disable_coms, CTL_BHT_COMREADDELAY, CTL_BHT_COMPORT, CTL_BHT_BAUDRATE, CTL_BHT_TIMEOUT, CTL_BHT_MAXTRIES, 0L);
	disableCtrls(disable_path, CTL_BHT_IMPEXPPATH, CTLBRW_BHT_IMPEXPPATH, 0);
	enableCommand(cmaMore, v == PPObjBHT::btStyloBhtII);
}

int BhtDialog::setDTS(const PPBhtTerminalPacket * pData)
{
	Data = *pData;

	ushort v = 0;
	setCtrlData(CTL_BHT_NAME, Data.Rec.Name);
	setCtrlData(CTL_BHT_ID, &Data.Rec.ID);
	AddClusterAssoc(CTL_BHT_TYPE,  0, PPObjBHT::btDenso);
	AddClusterAssoc(CTL_BHT_TYPE,  1, PPObjBHT::btSyntech);
	AddClusterAssoc(CTL_BHT_TYPE,  2, PPObjBHT::btPalm);
	AddClusterAssoc(CTL_BHT_TYPE,  3, PPObjBHT::btWinCe);
	AddClusterAssoc(CTL_BHT_TYPE,  4, PPObjBHT::btCom);
	AddClusterAssocDef(CTL_BHT_TYPE, 5, PPObjBHT::btStyloBhtII);
	SetClusterData(CTL_BHT_TYPE, Data.Rec.BhtTypeID);
	if(!oneof4(Data.Rec.BhtTypeID, PPObjBHT::btPalm, PPObjBHT::btStyloBhtII, PPObjBHT::btWinCe, PPObjBHT::btCom)) {
		int    c = 0;
		if(IsComDvcSymb(Data.Rec.Port, &c) == comdvcsCom) {
			if(c == 1)
				v = 0;
			else if(c == 2)
				v = 1;
			else if(c == 3)
				v = 2;
			else
				v = 0;
		}
		else
			v = 0;
		setCtrlData(CTL_BHT_COMPORT, &v);
		if(Data.Rec.Cbr == cbr9600)
			v = 0;
		else if(Data.Rec.Cbr == cbr19200)
			v = 1;
		else
			v = 1;
		setCtrlData(CTL_BHT_BAUDRATE, &v);
		setCtrlData(CTL_BHT_COMREADDELAY, &Data.Rec.ComGet_Delay);
		setCtrlData(CTL_BHT_TIMEOUT,  &Data.Rec.BhtpTimeout);
		setCtrlData(CTL_BHT_MAXTRIES, &Data.Rec.BhtpMaxTries);
	}
	else if(Data.Rec.BhtTypeID != PPObjBHT::btCom)
		setCtrlString(CTL_BHT_IMPEXPPATH, Data.ImpExpPath_);
	SetupLocationCombo(this, CTLSEL_BHT_LOC, Data.Rec.LocID, 0, LOCTYP_WAREHOUSE, 0); // @v8.4.2
	SetupPPObjCombo(this, CTLSEL_BHT_INVENTOP, PPOBJ_OPRKIND, Data.Rec.InventOpID, 0, (void *)PPOPT_INVENTORY);
	PPIDArray op_type_list;
	op_type_list.addzlist(PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND, 0L);
	SetupOprKindCombo(this, CTLSEL_BHT_EXPENDOP, Data.Rec.ExpendOpID,    0, &op_type_list, 0);
	SetupOprKindCombo(this, CTLSEL_BHT_INTROP,   Data.Rec.IntrExpndOpID, 0, &op_type_list, 0);
	v = (Data.Rec.ReceiptPlace == RCPTPLACE_ALTGROUP) ? 0 : 1;
	setCtrlData(CTL_BHT_RCPTPLACE, &v);
	AddClusterAssoc(CTL_BHT_FLAGS, 0, PPBhtTerminal::fDelAfterImport);
	SetClusterData(CTL_BHT_FLAGS, Data.Rec.Flags);
	DisableCtrls();
	return 1;
}

int BhtDialog::getDTS(PPBhtTerminalPacket * pData)
{
	int    ok = 1;
	ushort v = 0;
	getCtrlData(CTL_BHT_NAME, Data.Rec.Name);
	getCtrlData(CTL_BHT_ID, &Data.Rec.ID);
	GetClusterData(CTL_BHT_TYPE, &Data.Rec.BhtTypeID);
	if(!oneof4(Data.Rec.BhtTypeID, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btCom, PPObjBHT::btStyloBhtII)) {
		SString temp_buf;
		getCtrlData(CTL_BHT_COMPORT, &(v = 0));
		int    c = 1;
		if(v == 0)
			c = 1;
		else if(v == 1)
			c = 2;
		else if(v == 2)
			c = 3;
		GetComDvcSymb(comdvcsCom, c, 0, temp_buf).CopyTo(Data.Rec.Port, sizeof(Data.Rec.Port));
		getCtrlData(CTL_BHT_BAUDRATE, &(v = 0));
		if(v == 0)
			Data.Rec.Cbr = cbr9600;
		else if(v == 1)
			Data.Rec.Cbr = cbr19200;
		getCtrlData(CTL_BHT_COMREADDELAY, &Data.Rec.ComGet_Delay);
		getCtrlData(CTL_BHT_TIMEOUT,  &Data.Rec.BhtpTimeout);
		getCtrlData(CTL_BHT_MAXTRIES, &Data.Rec.BhtpMaxTries);
	}
	else if(Data.Rec.BhtTypeID != PPObjBHT::btCom) {
		getCtrlString(CTL_BHT_IMPEXPPATH, Data.ImpExpPath_);
		if(Data.ImpExpPath_.NotEmptyS()) {
			SPathStruc ps(Data.ImpExpPath_);
			if(ps.Drv.Empty())
				ok = PPSetError(PPERR_IMPEXPPATHNOTVALID);
		}
		else
			ok = PPSetError(PPERR_IMPEXPPATHNOTVALID);
	}
	THROW_PP(Data.Rec.BhtTypeID != PPObjBHT::btStyloBhtII || Data.P_SBIICfg && Data.P_SBIICfg->IsValid(), PPERR_SBII_CFGNOTVALID);
	getCtrlData(CTLSEL_BHT_LOC, &Data.Rec.LocID); // @v8.4.2
	getCtrlData(CTLSEL_BHT_INVENTOP, &Data.Rec.InventOpID);
	getCtrlData(CTLSEL_BHT_EXPENDOP, &Data.Rec.ExpendOpID);
	getCtrlData(CTLSEL_BHT_INTROP,   &Data.Rec.IntrExpndOpID);
	getCtrlData(CTL_BHT_RCPTPLACE, &v);
	Data.Rec.ReceiptPlace = (v == 0) ? RCPTPLACE_ALTGROUP : RCPTPLACE_GBASKET;
	GetClusterData(CTL_BHT_FLAGS, &Data.Rec.Flags);
	*pData = Data;
	CATCHZOK
	return ok;
}

void BhtDialog::editGoodsFilt()
{
	GoodsFilt flt;
	if(GoodsFilterDialog(Data.P_Filt ? Data.P_Filt : &flt) > 0)
		if(!Data.P_Filt) {
			Data.P_Filt = new GoodsFilt(flt);
			if(!Data.P_Filt)
				PPError(PPERR_NOMEM, 0);
		}
}
//
// StyloBhtIIOpInfoDlg
//
class StyloBhtIIOpInfoDlg : public PPListDialog {
public:
	StyloBhtIIOpInfoDlg() : PPListDialog(DLG_SBIIRESTR, CTL_SBIIRESTR_ERRLIST), PrevID(0)
	{
		setSmartListBoxOption(CTL_SBIIRESTR_ERRLIST, lbtSelNotify);
		PPLoadText(PPTXT_SBIIERRORS, ErrList);
	}
	int setDTS(const SBIIOpInfo *);
	int getDTS(SBIIOpInfo *);
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmLBItemSelected)) {
			long   id = 0;
			getSelection(&id);
			SetupCtrls(id);
			clearEvent(event);
		}
	}
	virtual int setupList();
	void   SetupCtrls(long newID);

	SString ErrList;
	long PrevID;
	SBIIOpInfo Data;
};

#define ACTION_CONFIRM 1L
#define ACTION_CANCEL  2L
#define ACTION_IGNORE  3L

void StyloBhtIIOpInfoDlg::SetupCtrls(long newID)
{
	long action = 0, flag = 0;
	if(PrevID != newID) {
		if(PrevID) {
			flag = 1 << (PrevID - 1);
			GetClusterData(CTL_SBIIRESTR_ACTION, &action);
			SETFLAG(Data.BhtCfmActions, flag, action == ACTION_CONFIRM);
			SETFLAG(Data.BhtOkCancelActions, flag, (action == ACTION_IGNORE));
		}
		if(newID) {
			flag = 1 << (newID - 1);
			action = (Data.BhtCfmActions & flag) ? ACTION_CONFIRM : 0;
			if(!action)
				action = (Data.BhtOkCancelActions & flag) ? ACTION_IGNORE : ACTION_CANCEL;
			SetClusterData(CTL_SBIIRESTR_ACTION, action);
			PrevID = newID;
		}
	}
}

// virtual
int StyloBhtIIOpInfoDlg::setupList()
{
	int    ok = 1;
	SString buf;
	StringSet ss(';', ErrList);
	for(uint i = 0; ss.get(&i, buf.Z()) > 0;) {
		long id = 0;
		uint j = 0;
		StringSet ss1(',', buf);
		ss1.get(&j, buf.Z());
		id = buf.ToLong();
		ss1.get(&j, buf.Z());
		THROW(addStringToList(id, buf.cptr()));
	}
	CATCHZOKPPERR
	return ok;
}

int StyloBhtIIOpInfoDlg::setDTS(const SBIIOpInfo * pData)
{
	if(!RVALUEPTR(Data, pData))
		MEMSZERO(Data);
	SetupOprKindCombo(this, CTLSEL_SBIIRESTR_HOSTOP, Data.ToHostOpID, OLW_CANEDIT, 0, 0);
	AddClusterAssocDef(CTL_SBIIRESTR_ACTION,  0, ACTION_CONFIRM);
	AddClusterAssoc(CTL_SBIIRESTR_ACTION,  1, ACTION_CANCEL);
	AddClusterAssoc(CTL_SBIIRESTR_ACTION,  2, ACTION_IGNORE);
	//
	AddClusterAssoc(CTL_SBIIRESTR_BHTOP, 0, StyloBhtIIConfig::oprkExpend);
	AddClusterAssoc(CTL_SBIIRESTR_BHTOP, 1, StyloBhtIIConfig::oprkReceipt);
	AddClusterAssoc(CTL_SBIIRESTR_BHTOP, 2, StyloBhtIIConfig::oprkTransfer);
	SetClusterData(CTL_SBIIRESTR_BHTOP, Data.ToBhtOpID);
	//
	AddClusterAssoc(CTL_SBIIRESTR_FLAGS, 0, StyloBhtIIConfig::foprCostAsPrice);
	AddClusterAssoc(CTL_SBIIRESTR_FLAGS, 1, StyloBhtIIConfig::foprUseDueDate);
	SetClusterData(CTL_SBIIRESTR_FLAGS, Data.Flags);
	disableCtrls(Data.OpID < 0, CTL_SBIIRESTR_ACTION, CTL_SBIIRESTR_ERRLIST, CTL_SBIIRESTR_BHTOP, 0L);
	updateList(-1);
	{
		SString buf;
		{
			StringSet ss(';', ErrList);
			ss.get(0U, buf);
		}
		{
			StringSet ss2(',', buf);
			ss2.get(0U, buf);
		}
		SetupCtrls(buf.ToLong());
	}
	return 1;
}

int StyloBhtIIOpInfoDlg::getDTS(SBIIOpInfo * pData)
{
	int    ok = 1;
	uint   sel = 0;
	getCtrlData(sel = CTLSEL_SBIIRESTR_HOSTOP, &Data.ToHostOpID);
	THROW_PP(Data.ToHostOpID, PPERR_INVOP);
	GetClusterData(CTL_SBIIRESTR_BHTOP,  &Data.ToBhtOpID);
	GetClusterData(CTL_SBIIRESTR_FLAGS,  &Data.Flags);
	SetupCtrls(0);
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = (selectCtrl(sel), 0);
	ENDCATCH
	return ok;
}
//
// StyloBhtIICfgDialog
//
class StyloBhtIICfgDialog : public PPListDialog {
public:
	StyloBhtIICfgDialog() : PPListDialog(DLG_SBIICFG, CTL_SBIICFG_OPLIST)
	{
		PPSecur item;
		PPObjSecur usr_obj(PPOBJ_USR, 0);
		SetupCalPeriod(CTLCAL_SBIICFG_DOCPRD, CTL_SBIICFG_DOCPRD);
		for(long id = 0; usr_obj.EnumItems(&id, &item) > 0;)
			UserList.Add(item.ID, item.ParentID, item.Name);
	}
	int setDTS(const StyloBhtIIOnHostCfg * pData);
	int getDTS(StyloBhtIIOnHostCfg * pData);
private:
	virtual int setupList();
	virtual int addItem(long *pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);

	StrAssocArray UserList;
	StyloBhtIIOnHostCfg Data;
	PPObjOprKind OprkObj;
};

// virtual
int StyloBhtIICfgDialog::setupList()
{
	int    ok = -1;
	if(Data.P_OpList) {
		SString buf;
		for(uint i = 0; i < Data.P_OpList->getCount(); i++) {
			buf.Z();
			PPID   op_id = Data.P_OpList->at(i).OpID;
			if(op_id < 0) {
				if(op_id == -1)
					PPGetWord(PPWORD_NOTLINKEXPEND, 0, buf);
				else if(op_id == -2)
					PPGetWord(PPWORD_NOTLINKRECEIPT, 0, buf);
				else
					PPGetWord(PPWORD_NOTLINKTRANSFER, 0, buf);
			}
			else
				GetObjectName(PPOBJ_OPRKIND, op_id, buf, 0);
			THROW(addStringToList(i, buf));
		}
	}
	CATCHZOKPPERR
	return ok;
}

// virtual
int StyloBhtIICfgDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	PPID   op_id = 0;
	SString obj_title;
	StrAssocArray * p_list = OprkObj.MakeStrAssocList(0);
	StrAssocArray _list;
	if(p_list && p_list->getCount()) {
		p_list->SortByText();
		PPIDArray op_type_list;
		op_type_list.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN, PPOPT_GOODSREVAL,
			PPOPT_GOODSORDER, PPOPT_GOODSMODIF, PPOPT_GOODSACK, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, 0L);
		for(uint i = 0; i < p_list->getCount(); i++) {
			StrAssocArray::Item item = p_list->Get(i);
			if(!Data.P_OpList->lsearch(&item.Id, 0, PTR_CMPFUNC(long)) && op_type_list.lsearch(GetOpType(item.Id)))
				_list.Add(item.Id, item.Txt);
		}
		GetObjectTitle(PPOBJ_OPRKIND, obj_title);
		if(ListBoxSelDialog(&_list, obj_title, &op_id, 0) > 0) {
			long   pos = 0;
			SBIIOpInfo op_info;
			MEMSZERO(op_info);
			THROW_MEM(Data.P_OpList);
			op_info.OpID = op_id;
			THROW_SL(Data.P_OpList->insert(&op_info));
			pos = (long)Data.P_OpList->getCount() - 1;
			if(editItem(pos, op_id) > 0)
				ok = 1;
			else
				Data.P_OpList->atFree(pos);
		}
	}
	CATCHZOKPPERR
	ZDELETE(p_list);
	return ok;
}

// virtual
int StyloBhtIICfgDialog::editItem(long pos, long id)
{
	int    ok = -1, valid_data = 0;
	StyloBhtIIOpInfoDlg * dlg = 0;
	if(Data.P_OpList && pos >= 0 && pos < (long)Data.P_OpList->getCount()) {
		if(CheckDialogPtrErr(&(dlg = new StyloBhtIIOpInfoDlg)) > 0) {
			dlg->setDTS(&Data.P_OpList->at(pos));
			while(!valid_data && ExecView(dlg) == cmOK) {
				if(dlg->getDTS(&Data.P_OpList->at(pos)) > 0)
					ok = valid_data = 1;
				else
					PPError();
			}
		}
	}
	delete dlg;
	return ok;
}

// virtual
int StyloBhtIICfgDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(Data.P_OpList && pos < (long)Data.P_OpList->getCount() && Data.P_OpList->at(pos).OpID > 0 && CONFIRM(PPCFM_DELITEM)) {
		Data.P_OpList->atFree(pos);
		ok = 1;
	}
	return ok;
}

int StyloBhtIICfgDialog::setDTS(const StyloBhtIIOnHostCfg * pData)
{
	SString buf;
	PPID   user_id = 0;
	if(!RVALUEPTR(Data, pData))
		Data.Init();
	{
		PPGoodsConfig goods_cfg;
		PPObjGoods::ReadConfig(&goods_cfg);
		Data.WeightPrefix     = goods_cfg.WghtPrefix;
		Data.QttyWeightPrefix = goods_cfg.WghtCntPrefix;
	}
	setCtrlString(CTL_SBIICFG_DEVICENAME,  Data.DeviceName);
	if(Data.UserName.Len()) {
		uint pos = 0;
		if(UserList.SearchByText(Data.UserName, 0, &pos) <= 0) {
			PPSetAddedMsgString(Data.UserName);
			PPError(PPERR_USERNOTFOUND);
		}
		else
			user_id = UserList.Get(pos).Id;
	}
	SetupStrAssocCombo(this, CTLSEL_SBIICFG_USER, &UserList, user_id, 0, 0, 0);
	InetAddr::ULongToIP(Data.ServerAddr, buf);
	setCtrlString(CTL_SBIICFG_SERVADDR,    buf);
	InetAddr::ULongToIP(Data.ServerMask, buf);
	setCtrlString(CTL_SBIICFG_SERVMASK,    buf);
	setCtrlData(CTL_SBIICFG_SERVPORT,    &Data.ServerPort);
	setCtrlData(CTL_SBIICFG_DEFQTTY,     &Data.DefQtty);
	setCtrlString(CTL_SBIICFG_WEIGHTPREFX, Data.WeightPrefix);
	AddClusterAssoc(CTL_SBIICFG_FLAGS, 0, StyloBhtIIConfig::fExitByEsc);
	AddClusterAssoc(CTL_SBIICFG_FLAGS, 1, StyloBhtIIConfig::fUseWiFi);
	AddClusterAssoc(CTL_SBIICFG_FLAGS, 2, StyloBhtIIConfig::fUseDefQtty);
	AddClusterAssoc(CTL_SBIICFG_FLAGS, 3, StyloBhtIIConfig::fCheckQtty);
	AddClusterAssoc(CTL_SBIICFG_FLAGS, 4, StyloBhtIIConfig::fCheckPrice);
	AddClusterAssoc(CTL_SBIICFG_FLAGS, 5, StyloBhtIIConfig::fCheckExpiry);
	AddClusterAssoc(CTL_SBIICFG_FLAGS, 6, StyloBhtIIConfig::fAllowUnknownGoods);
	AddClusterAssoc(CTL_SBIICFG_FLAGS, 7, StyloBhtIIConfig::fInputBillRowNumber);
	SetClusterData(CTL_SBIICFG_FLAGS, Data.Flags);
	SetPeriodInput(this, CTL_SBIICFG_DOCPRD, &Data.ExportBillsPeriod);
	SetupPPObjCombo(this, CTLSEL_SBIICFG_PRINTER, PPOBJ_BCODEPRINTER, Data.BcdPrinterID, OLW_CANEDIT);
	disableCtrl(CTL_SBIICFG_WEIGHTPREFX, 1);
	updateList(-1);
	return 1;
}

int StyloBhtIICfgDialog::getDTS(StyloBhtIIOnHostCfg * pData)
{
	int    ok = -1;
	uint   sel = 0;
	PPID   user_id = 0;
	SString buf;
	getCtrlString(sel = CTL_SBIICFG_DEVICENAME,  Data.DeviceName);
	THROW_PP(Data.DeviceName.Len(), PPERR_NAMENEEDED);
	Data.UserName = 0;
	Data.Password = 0;
	getCtrlData(CTL_SBIICFG_USER, &user_id);
	UserList.GetText(user_id, Data.UserName);
	if(user_id) {
		PPSecurPacket secur_pack;
		PPRef->LoadSecur(PPOBJ_USR, user_id, &secur_pack);
		Data.Password.CopyFrom(secur_pack.Secur.Password);
	}
	getCtrlString(CTL_SBIICFG_SERVADDR,    buf);
	Data.ServerAddr = InetAddr::IPToULong(buf);
	getCtrlString(CTL_SBIICFG_SERVMASK,    buf);
	Data.ServerMask = InetAddr::IPToULong(buf);
	getCtrlData(CTL_SBIICFG_SERVPORT,     &Data.ServerPort);
	getCtrlData(CTL_SBIICFG_DEFQTTY,     &Data.DefQtty);
	getCtrlString(CTL_SBIICFG_WEIGHTPREFX, Data.WeightPrefix);
	GetClusterData(CTL_SBIICFG_FLAGS,     &Data.Flags);
	if(Data.Flags & StyloBhtIIConfig::fUseWiFi) {
		sel = CTL_SBIICFG_USER;
		THROW_PP(Data.UserName.Len(), PPERR_INVUSERORPASSW);
		THROW_PP(Data.Password.Len(), PPERR_INVUSERORPASSW);
		sel = CTL_SBIICFG_SERVADDR;
		THROW_PP(Data.ServerAddr > 0, PPERR_INVIP);
	}
	GetPeriodInput(this, CTL_SBIICFG_DOCPRD, &Data.ExportBillsPeriod);
	getCtrlData(CTLSEL_SBIICFG_PRINTER, &Data.BcdPrinterID);
	THROW(Data.IsValid());
	ASSIGN_PTR(pData, Data);
	ok = 1;
	CATCH
		ok = (selectCtrl(sel), 0);
	ENDCATCH
	return ok;
}

void BhtDialog::editMore()
{
	int    valid_data = 0;
	StyloBhtIICfgDialog * dlg = new StyloBhtIICfgDialog;
	SETIFZ(Data.P_SBIICfg, new StyloBhtIIOnHostCfg);
	THROW_MEM(Data.P_SBIICfg);
	SETIFZ(Data.P_SBIICfg->P_OpList, new SBIIOpInfoArray);
	THROW_MEM(Data.P_SBIICfg->P_OpList);
	for(long op_id = -StyloBhtIIConfig::oprkTransfer; op_id <= -StyloBhtIIConfig::oprkExpend; op_id++) {
		if(Data.P_SBIICfg->P_OpList->lsearch(&op_id, 0, PTR_CMPFUNC(long)) <= 0) {
			SBIIOpInfo op_info;
			MEMSZERO(op_info);
			op_info.OpID = op_id;
			op_info.BhtOkCancelActions = 0xFFFFFFFF;
			op_info.ToBhtOpID = labs(op_info.OpID);
			THROW_SL(Data.P_SBIICfg->P_OpList->insert(&op_info));
		}
	}
	THROW(CheckDialogPtr(&dlg));
	dlg->setDTS(Data.P_SBIICfg);
	while(!valid_data && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(Data.P_SBIICfg) > 0)
			valid_data = 1;
		else
			PPError();
	}
	CATCH
		PPError();
	ENDCATCH;
	if(Data.P_SBIICfg && Data.P_SBIICfg->IsEmpty())
		ZDELETE(Data.P_SBIICfg);
	delete dlg;
}

int SLAPI PPObjBHT::Edit(PPID * pID, void * extraPtr)
{
	int    ok = -1, r = cmCancel, valid_data = 0, is_new = 0;
	BhtDialog * dlg = 0;
	PPBhtTerminalPacket pack;
	THROW(CheckDialogPtr(&(dlg = new BhtDialog())));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	else {
		SString temp_buf;
		for(int i = 1; i <= 32; i++) {
			temp_buf.Z().Cat("BHT").Space().CatChar('#').Cat(i).CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
			if(CheckDupName(*pID, pack.Rec.Name))
				break;
			else
				memzero(pack.Rec.Name, sizeof(pack.Rec.Name));
		}
		GetComDvcSymb(comdvcsCom, 1, 0, temp_buf).CopyTo(pack.Rec.Port, sizeof(pack.Rec.Port));
		pack.Rec.Cbr = cbr19200;
		pack.Rec.ComGet_Delay = 75;
		pack.Rec.BhtpTimeout = 3000;
		pack.Rec.BhtpMaxTries = 10;
	}
	dlg->setDTS(&pack);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		if(dlg->getDTS(&pack)) {
			if(!CheckName(*pID, pack.Rec.Name, 0))
				dlg->selectCtrl(CTL_SCALE_NAME);
			else if(!PutPacket(pID, &pack, 1))
				PPError();
			else
				valid_data = 1;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}
//
//
//
struct __StyloBhtIIConfig { // @persistent
	PPID   ObjType;
	PPID   ObjID;
	PPID   PropID;
	char   DeviceName[32];
	int32  Flags;
	char   WeightPrefix[32];
	char   UserName[32];
	char   Password[128];
	ulong  ServerAddr;
	long   ServerPort;
	ulong  ServerMask;
	DateRange ExportBillsPeriod;
	double DefQtty;
	PPID   BcdPrinterID;
	char   QttyWeightPrefix[32];  // !!! ���� ��������� � persistent ��������� ��� ������
	char   Reserve[508];
	// P_OpList
};

struct __StyloBhtIIConfig__ { // @persistent
	PPID   ObjType;
	PPID   ObjID;
	PPID   PropID;
	char   DeviceName[32];
	int32  Flags;
	char   WeightPrefix[32];
	char   UserName[32];
	char   Password[128];
	ulong  ServerAddr;
	long   ServerPort;
	ulong  ServerMask;
	DateRange ExportBillsPeriod;
	double DefQtty;
	PPID   BcdPrinterID;
	char   Reserve[508];
	// P_OpList
};

struct __StyloBhtConfig2 {
	enum {
		efDeviceName = 1,
		efUserName,
		efPassword,
		efWghtPrefix,
		efWghtCntPrefix,
		efImpExpPath
	};
	PPID   ObjType;
	PPID   ObjID;
	PPID   PropID;

	long   Flags;
	uint32 ServerAddr;
	uint32 ReserveAddr;
	int32  ServerPort;
	uint32 ServerMask;
	DateRange ExportBillsPeriod;
	double DefQtty;
	SVerT Ver;                  // ������ �������, �������� ������ � ��
	uint32 OpItemsCount;         // ���������� ��������� �������� ��������
	uint32 ExtStrLen;            // ����� ������, ���������� ��������� ���������� ���������
	uint8  Reserve[16];
	PPID   BcdPrinterID;
	long   Reserve2;
	// SBIIOpInfo[OpItemsCount]
	// string[ExtStrLen]
};

int SLAPI PPObjBHT::GetPacket(PPID id, PPBhtTerminalPacket * pPack)
{
	int    ok = 1;
	__StyloBhtConfig2 * p_buf = 0;
	__StyloBhtIIConfig * p_buf_pre764 = 0;
	ZDELETE(pPack->P_Filt);
	ZDELETE(pPack->P_SBIICfg);
	if(Search(id, &pPack->Rec) > 0) {
		StyloBhtIIOnHostCfg * p_cfg = 0;
		GoodsFilt flt;
		size_t cfg_indb_size = 0;
		int   _pre764 = 0; // 1 - pre764 ��� QttyWeightPrefix, 2 - pre764 � QttyWeightPrefix, -1 - �� ������� ����������������
		if(flt.ReadFromProp(PPOBJ_BHT, id, BHTPRP_GOODSFILT2, BHTPRP_GOODSFILT_) > 0) {
			if(!flt.IsEmpty())
				THROW_MEM(pPack->P_Filt = new GoodsFilt(flt));
		}
		if(ref->GetPropActualSize(Obj, id, BHTPRP_SBIICFG, &cfg_indb_size) > 0) {
			const uint __sz = sizeof(__StyloBhtIIConfig);
			const uint __sz__ = sizeof(__StyloBhtIIConfig__);
			if(((cfg_indb_size - __sz__ - sizeof(uint32)) % sizeof(SBIIOpInfo)) == 0) {
				_pre764 = 1;
			}
			else if(((cfg_indb_size - __sz - sizeof(uint32)) % sizeof(SBIIOpInfo)) == 0) {
				_pre764 = 2;
			}
			else {
				_pre764 = -1;
				CALLEXCEPT_PP(PPERR_BHTPACKPROPREADFAULT_764);
			}
			if(oneof2(_pre764, 1, 2)) {
				THROW_MEM(p_buf_pre764 = (__StyloBhtIIConfig *)SAlloc::M(cfg_indb_size));
				memzero(p_buf_pre764, cfg_indb_size);
				THROW(ref->GetProperty(Obj, id, BHTPRP_SBIICFG, p_buf_pre764, cfg_indb_size) > 0);
				{
					uint8 * ptr = 0;
					uint   op_items_count = 0;
					if(_pre764 == 1) {
						ptr = PTR8(p_buf_pre764) + sizeof(__StyloBhtIIConfig__);
					}
					else {
						ptr = PTR8(p_buf_pre764) + sizeof(__StyloBhtIIConfig);
					}
					op_items_count = PTR32(ptr)[0];
					THROW_MEM(pPack->P_SBIICfg = new StyloBhtIIOnHostCfg);
					p_cfg = pPack->P_SBIICfg;
					p_cfg->DeviceName = p_buf_pre764->DeviceName;
					p_cfg->Flags = p_buf_pre764->Flags;
					p_cfg->WeightPrefix = p_buf_pre764->WeightPrefix;
					p_cfg->UserName = p_buf_pre764->UserName;
					p_cfg->Password = p_buf_pre764->Password;
					p_cfg->ServerAddr        = p_buf_pre764->ServerAddr;
					p_cfg->ServerPort        = p_buf_pre764->ServerPort;
					p_cfg->ServerMask        = p_buf_pre764->ServerMask;
					p_cfg->ExportBillsPeriod = p_buf_pre764->ExportBillsPeriod;
					p_cfg->DefQtty           = p_buf_pre764->DefQtty;
					p_cfg->BcdPrinterID      = p_buf_pre764->BcdPrinterID;
					{
						PPGoodsConfig goods_cfg;
						PPObjGoods::ReadConfig(&goods_cfg);
						p_cfg->QttyWeightPrefix = goods_cfg.WghtCntPrefix;
					}
					if(op_items_count) {
						THROW_MEM(p_cfg->P_OpList = new SBIIOpInfoArray);
						for(uint i = 0; i < op_items_count; i++) {
							THROW_SL(p_cfg->P_OpList->insert(((SBIIOpInfo *)(ptr + sizeof(uint32))) + i));
						}
					}
				}
			}
		}
		else {
			uint   cfg_rec_size = 4096;
			THROW_MEM(p_buf = (__StyloBhtConfig2 *)SAlloc::M(cfg_rec_size));
			memzero(p_buf, cfg_rec_size);
			if(ref->GetProperty(Obj, id, BHTPRP_SBIICFG2, p_buf, cfg_rec_size) > 0) {
				THROW_MEM(pPack->P_SBIICfg = new StyloBhtIIOnHostCfg);
				p_cfg = pPack->P_SBIICfg;
				p_cfg->Flags = p_buf->Flags;
				p_cfg->ServerAddr = p_buf->ServerAddr;
				p_cfg->ServerPort = p_buf->ServerPort;
				p_cfg->ServerMask = p_buf->ServerMask;
				p_cfg->ExportBillsPeriod = p_buf->ExportBillsPeriod;
				p_cfg->DefQtty = p_buf->DefQtty;
				p_cfg->BcdPrinterID = p_buf->BcdPrinterID;
				{
					PPGoodsConfig goods_cfg;
					PPObjGoods::ReadConfig(&goods_cfg);
					p_cfg->QttyWeightPrefix = goods_cfg.WghtCntPrefix;
				}
				if(p_buf->OpItemsCount) {
					THROW_MEM(p_cfg->P_OpList = new SBIIOpInfoArray);
					for(uint i = 0; i < p_buf->OpItemsCount; i++) {
						THROW_SL(p_cfg->P_OpList->insert(((SBIIOpInfo *)(p_buf+1))+i));
					}
				}
				if(p_buf->ExtStrLen) {
					SString ext_str;
					ext_str.CatN((char *)(PTR8(p_buf+1) + p_buf->OpItemsCount * sizeof(SBIIOpInfo)), p_buf->ExtStrLen);
					PPGetExtStrData(__StyloBhtConfig2::efDeviceName, ext_str, p_cfg->DeviceName);
					PPGetExtStrData(__StyloBhtConfig2::efUserName,   ext_str, p_cfg->UserName);
					PPGetExtStrData(__StyloBhtConfig2::efPassword,   ext_str, p_cfg->Password);
					PPGetExtStrData(__StyloBhtConfig2::efWghtPrefix, ext_str, p_cfg->WeightPrefix);
				}
			}
		}
		if(oneof3(pPack->Rec.BhtTypeID, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
			ref->GetPropVlrString(PPOBJ_BHT, id, BHTPRP_PATH, pPack->ImpExpPath_);
		}
	}
	else
		ok = -1;
	CATCHZOK
	SAlloc::F(p_buf);
	SAlloc::F(p_buf_pre764);
	return ok;
}

int SLAPI PPObjBHT::PutPacket(PPID * pID, PPBhtTerminalPacket * pPack, int use_ta)
{
	int    ok = 1;
	__StyloBhtConfig2 * p_buf = 0;
	PPTransaction tra(use_ta);
	THROW(tra);
	if(*pID) {
		THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
	}
	else {
		*pID = pPack->Rec.ID;
		THROW(ref->AddItem(Obj, pID, &pPack->Rec, 0));
	}
	if(pPack->P_Filt) {
		THROW(pPack->P_Filt->WriteToProp(Obj, *pID, BHTPRP_GOODSFILT2, BHTPRP_GOODSFILT_));
	}
	else {
		THROW(ref->PutProp(Obj, *pID, BHTPRP_GOODSFILT2, 0));
		THROW(ref->PutProp(Obj, *pID, BHTPRP_GOODSFILT_, 0));
	}
	if(pPack->P_SBIICfg && pPack->Rec.BhtTypeID == PPObjBHT::btStyloBhtII) {
		StyloBhtIIOnHostCfg * p_cfg = pPack->P_SBIICfg;
		SString ext_str;
		PPPutExtStrData(__StyloBhtConfig2::efDeviceName, ext_str, p_cfg->DeviceName);
		PPPutExtStrData(__StyloBhtConfig2::efUserName, ext_str, p_cfg->UserName);
		PPPutExtStrData(__StyloBhtConfig2::efPassword, ext_str, p_cfg->Password);
		PPPutExtStrData(__StyloBhtConfig2::efWghtPrefix, ext_str, p_cfg->WeightPrefix);

		const  uint    op_items_count = p_cfg->P_OpList ? p_cfg->P_OpList->getCount() : 0;
		const  size_t  buf_size = sizeof(*p_buf) + op_items_count * sizeof(SBIIOpInfo) + ext_str.Len();
		char * p = 0;

		THROW_MEM(p_buf = (__StyloBhtConfig2 *)SAlloc::M(buf_size));
		memzero(p_buf, buf_size);

		p_buf->ObjType = PPOBJ_BHT;
		p_buf->ObjID   = *pID;
		p_buf->PropID  = BHTPRP_SBIICFG2;
		p_buf->Flags   = p_cfg->Flags;
		p_buf->ServerAddr = p_cfg->ServerAddr;
		p_buf->ServerPort = p_cfg->ServerPort;
		p_buf->ServerMask = p_cfg->ServerMask;
		p_buf->ExportBillsPeriod = p_cfg->ExportBillsPeriod;
		p_buf->DefQtty = p_cfg->DefQtty;
		p_buf->BcdPrinterID = p_cfg->BcdPrinterID;
		p_buf->Ver = DS.GetVersion();
		p_buf->OpItemsCount = op_items_count;
		p_buf->ExtStrLen = (uint32)ext_str.Len();

		for(uint i = 0; i < op_items_count; i++) {
			((SBIIOpInfo *)(p_buf+1))[i] = pPack->P_SBIICfg->P_OpList->at(i);
		}
		if(ext_str.Len()) {
			memcpy(PTR8(p_buf+1) + op_items_count * sizeof(SBIIOpInfo), ext_str.cptr(), ext_str.Len());
		}
		THROW(ref->PutProp(Obj, *pID, BHTPRP_SBIICFG2, p_buf, buf_size));
	}
	else {
		THROW(ref->RemoveProperty(Obj, *pID, BHTPRP_SBIICFG2, 0));
	}
	THROW(ref->RemoveProperty(Obj, *pID, BHTPRP_SBIICFG, 0)); // ������� ������� ������ ������
	THROW(ref->PutPropVlrString(PPOBJ_BHT, *pID, BHTPRP_PATH,
		oneof3(pPack->Rec.BhtTypeID, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII) ? pPack->ImpExpPath_ : (const char *)0));
	THROW(tra.Commit());
	CATCHZOK
	SAlloc::F(p_buf);
	return ok;
}

class BhtRecord {
public:
	SLAPI  BhtRecord();
	SLAPI  BhtRecord(const BhtRecord &);
	void   SLAPI Reset();
	void   SLAPI Init(const BhtRecord &);
	int    SLAPI AddFld(uint);
	int    SLAPI PutInt(uint fldNo, long);
	int    SLAPI PutStr(uint fldNo, const char *);
	int    SLAPI PutDbl(uint fldNo, uint prec, double);
	int    SLAPI GetInt(uint fldNo, long *);
	void   SLAPI GetStr(uint fldNo, char *, size_t);
	void   SLAPI GetDbl(uint fldNo, double *);
	size_t SLAPI FldOfs(uint fldNo) const;
	size_t SLAPI GetBufLen() const;
	const  char * GetBuf() const;
	void   SLAPI GetHeader(const char * pFileName, uint numRecs, size_t * pDataLen, char * pBuf) const;
	void   SLAPI SetHeader(const char * pBuf);
	int    SLAPI SetBuf(size_t dataSize, const char * pBuf);
private:
	uint   NumFlds;
	uint   Lens[16];
	char   Buf[256];
};

class BhtFile {
public:
	SLAPI  BhtFile(const char * pFileName);
	SLAPI ~BhtFile();
	int    SLAPI Init(const char * pFileName);
	int    SLAPI GetNumRecs(uint * pNumRecs);
	int    SLAPI InitRecord(BhtRecord *) const;
	int    SLAPI EnumRecords(uint * pRecNo, BhtRecord *);

	FILE * Stream;
	char   Name[32];
	uint   NumRecs;
	BhtRecord * P_RecTmpl;
};

enum StyloBhtCmd {
	stybcStopSess = 1,
	stybcReceiveSuppl,
	stybcReceiveGoods,
	stybcTransmitBill,
	stybcTransmitInvent
};

class BhtProtocol : public SCommPort {
public:
	SLAPI  BhtProtocol();
	SLAPI ~BhtProtocol();
	void   SLAPI SetProtParams(uint timeout, uint maxTries, long flags);
	//
	// Sender
	//
	int    SLAPI SetConnection();
	int    SLAPI ReleaseConnection();
	int    SLAPI SendPrgmHeadingText(const char * pFileName, uint numRecs);
	int    SLAPI SendDataHeadingText(const char * pFileName, uint numRecs, const BhtRecord *);
	int    SLAPI SendPrgmText(uint recNo, size_t blkLen, const char *);
	int    SLAPI SendRecord(uint recNo, const BhtRecord *);
	//
	// Receiver
	//
	int    SLAPI WaitOnConnection(int releaseLink, long timeout);
	int    SLAPI ReceiveBlock(uint * pRecNo, size_t * pDataLen, char * pBuf, size_t bufLen);
	static void SLAPI ParseHeadingText(const char * pBuf, char * pFileName, uint * pNumRecs);
	//
	// High level
	//
	int    SLAPI SendPrgmFile(const char * pFileName);
	int    SLAPI SendDataFile(const char * pFileName, const BhtRecord *);
	int    SLAPI ReceiveFile(const char * pFileName, long timeout);

	int    SLAPI SendCommand(StyloBhtCmd);
private:
	int    SLAPI SendBlock(uint, size_t, const char * pBlk);
	int    SLAPI GetChr_();

	uint16 Timeout;
	uint16 MaxTries;
	long   Flags;
};

class CipherProtocol : public SCommPort {
public:
	SLAPI  CipherProtocol();
	SLAPI  ~CipherProtocol();
	void   SLAPI SetProtParams(uint timeout, uint maxTries, long flags);
	//
	// Sender
	//
	int    SLAPI SetConnection();
	int    SLAPI ReleaseConnection();
	int    SLAPI SendRecord(uint recNo, const BhtRecord *);
	int    SLAPI SendDataHeadingText(const char * pFileName, uint numRecs, const BhtRecord *);
	//
	// Receiver
	//
	int    SLAPI WaitOnConnection(int releaseLink, long timeout);
	int    SLAPI ReceiveBlock(uint * pRecNo, size_t * pDataLen, char * pBuf, size_t bufLen);
	static void SLAPI ParseHeadingText(const char * pBuf, char * pFileName, uint * pNumRecs);
	//
	// High level
	//
	int    SLAPI SendDataFile(const char * pFileName, const BhtRecord *);
	int    SLAPI ReceiveFile(const char * pFileName, long timeout);
private:
	int    SLAPI SendBlock(uint, size_t, const char * pBlk);
	int    SLAPI PutChrEx(int c);
	int    SLAPI GetChrEx();

	uint16 Timeout;
	uint16 MaxTries;
	long   Flags;
};
//
//
//
static void SLAPI GetIntFromBuf(long * pVal, int fldLen, const char * pBuf)
{
	char val_buf[32];
	for(int i = 0; i < fldLen; i++)
		val_buf[i] = pBuf[i];
	val_buf[fldLen] = 0;
	*pVal = atol(val_buf);
}

static void SLAPI GetStrFromBuf(char * pStr, size_t fldLen, const char * pBuf)
{
	/* @v8.6.6
	for(size_t i = 0; i < fldLen; i++)
		pStr[i] = pBuf[i];
	*/
	memmove(pStr, pBuf, fldLen); // @v8.6.6
	pStr[fldLen] = 0;
	trimright(pStr);
}

static int SLAPI PutIntToBuf(long val, size_t fldLen, char * pBuf)
{
	char tmp_buf[32];
	longfmtz(val, (int)fldLen, tmp_buf, sizeof(tmp_buf));
	strcpy(pBuf, tmp_buf);
	return (sstrlen(tmp_buf) > fldLen) ? 0 : 1;
}

static int SLAPI PutStrToBuf(const char * pStr, size_t fldLen, char * pBuf)
{
	size_t len = sstrlen(pStr);
	len = MIN(len, fldLen);
	memcpy(pBuf, pStr, len);
	memset(pBuf+len, ' ', fldLen-len);
	return 1;
}
//
// BhtRecord
//
SLAPI BhtRecord::BhtRecord()
{
	Reset();
}

SLAPI BhtRecord::BhtRecord(const BhtRecord & s)
{
	Init(s);
}

void SLAPI BhtRecord::Init(const BhtRecord & s)
{
	Reset();
	for(uint i = 0; i < s.NumFlds; i++)
		AddFld(s.Lens[i]);
}

void SLAPI BhtRecord::Reset()
{
	NumFlds = 0;
	memzero(Lens, sizeof(Lens));
	memzero(Buf, sizeof(Buf));
}

int SLAPI BhtRecord::AddFld(uint len)
{
	if(NumFlds < 16 && len < 100) {
		Lens[NumFlds++] = len;
		return 1;
	}
	else
		return 0;
}

size_t SLAPI BhtRecord::FldOfs(uint fldNo) const
{
	size_t ofs = 0;
	if(fldNo < NumFlds)
		for(uint i = 0; i < fldNo; i++)
			ofs += Lens[i];
	return ofs;
}

int SLAPI BhtRecord::PutInt(uint fldNo, long val)
{
	return (fldNo < NumFlds) ? PutIntToBuf(val, Lens[fldNo], Buf+FldOfs(fldNo)) : 0;
}

int SLAPI BhtRecord::PutStr(uint fldNo, const char * pStr)
{
	return (fldNo < NumFlds) ? PutStrToBuf(pStr, Lens[fldNo], Buf+FldOfs(fldNo)) : 0;
}

int SLAPI BhtRecord::PutDbl(uint fldNo, uint prec, double val)
{
	if(fldNo < NumFlds) {
		char tmp_buf[32];
		realfmt(val, MKSFMTD(0, prec, 0), tmp_buf);
		return PutStrToBuf(tmp_buf, Lens[fldNo], Buf+FldOfs(fldNo));
	}
	return 0;
}

int SLAPI BhtRecord::GetInt(uint fldNo, long * pVal)
{
	if(fldNo < NumFlds) {
		char   tmp_buf[128];
		memzero(tmp_buf, sizeof(tmp_buf));
		memcpy(tmp_buf, Buf+FldOfs(fldNo), Lens[fldNo]);
		*pVal = atol(strip(tmp_buf));
		return 1;
	}
	else
		return 0;
}

void SLAPI BhtRecord::GetStr(uint fldNo, char * pBuf, size_t bufLen)
{
	if(fldNo < NumFlds) {
		char tmp_buf[128];
		memzero(tmp_buf, sizeof(tmp_buf));
		memcpy(tmp_buf, Buf+FldOfs(fldNo), Lens[fldNo]);
		strnzcpy(pBuf, strip(tmp_buf), bufLen);
	}
}

void SLAPI BhtRecord::GetDbl(uint fldNo, double * pVal)
{
	if(fldNo < NumFlds) {
		char tmp_buf[128];
		memzero(tmp_buf, sizeof(tmp_buf));
		memcpy(tmp_buf, Buf+FldOfs(fldNo), Lens[fldNo]);
		*pVal = atof(strip(tmp_buf));
	}
}

size_t SLAPI BhtRecord::GetBufLen() const
{
	size_t s = 0;
	for(uint i = 0; i < NumFlds; i++)
		s += Lens[i];
	return s;
}

const char * BhtRecord::GetBuf() const
{
	return Buf;
}

void SLAPI BhtRecord::GetHeader(const char * pFileName, uint numRecs, size_t * pDataLen, char * pBuf) const
{
	size_t p = 0;
	PutStrToBuf(pFileName, FNAMELEN, pBuf+p); p += FNAMELEN;
	PutIntToBuf(numRecs, NUMRECSLEN, pBuf+p); p += NUMRECSLEN;
	PutIntToBuf(NumFlds, NUMFLDSLEN, pBuf+p); p += NUMFLDSLEN;
	for(uint i = 0; i < NumFlds; i++) {
		PutIntToBuf(Lens[i], FLDLENLEN, pBuf+p);
		p += FLDLENLEN;
	}
	ASSIGN_PTR(pDataLen, p);
}

void SLAPI BhtRecord::SetHeader(const char * pBuf)
{
	long   v = 0;
	Reset();
	GetIntFromBuf(&v, NUMFLDSLEN, pBuf+FNAMELEN+NUMRECSLEN);
	uint   num_flds = (uint)v;
	for(uint i = 0; i < num_flds; i++) {
		GetIntFromBuf(&v, FLDLENLEN, pBuf+FNAMELEN+NUMRECSLEN+NUMFLDSLEN+i*FLDLENLEN);
		AddFld((uint)v);
	}
}

int SLAPI BhtRecord::SetBuf(size_t dataSize, const char * pBuf)
{
	memcpy(Buf, pBuf, dataSize);
	return 1;
}
//
//
//
SLAPI BhtFile::BhtFile(const char * pFileName) : Stream(0), P_RecTmpl(0)
{
	Init(pFileName);
}

SLAPI BhtFile::~BhtFile()
{
	SFile::ZClose(&Stream);
	delete P_RecTmpl;
}

int SLAPI BhtFile::Init(const char * pFileName)
{
	int    ok = -1;
	SFile::ZClose(&Stream);
	ZDELETE(P_RecTmpl);
	memzero(Name, sizeof(Name));
	NumRecs = 0;
	if(pFileName) {
		char   line_buf[256];
		THROW_PP_S(Stream = fopen(pFileName, "r"), PPERR_CANTOPENFILE, pFileName);
		if(fgets(line_buf, sizeof(line_buf), Stream)) {
			BhtProtocol::ParseHeadingText(line_buf, Name, &NumRecs);
			strip(Name);
			if(Name[0] && NumRecs <= MAXBHTRECS) {
				THROW_MEM(P_RecTmpl = new BhtRecord);
				P_RecTmpl->SetHeader(line_buf);
				ok = 1;
			}
		}
	}
	CATCHZOK
	if(ok <= 0 && pFileName)
		Init(0);
	return ok;
}

int SLAPI BhtFile::InitRecord(BhtRecord * pRec) const
{
	return P_RecTmpl ? (pRec->Init(*P_RecTmpl), 1) : 0;
}

int SLAPI BhtFile::GetNumRecs(uint * pNumRecs)
{
	uint   num_recs = 0;
	if(Stream) {
		char   line_buf[256];
		rewind(Stream);
		while(fgets(line_buf, sizeof(line_buf), Stream))
			++num_recs;
		if(num_recs)
			--num_recs;
	}
	ASSIGN_PTR(pNumRecs, num_recs);
	return num_recs ? 1 : -1;
}

int SLAPI BhtFile::EnumRecords(uint * pRecNo, BhtRecord * pRec)
{
	if(Stream) {
		char   line_buf[256];
		rewind(Stream);
		for(uint i = 0; fgets(line_buf, sizeof(line_buf), Stream);)
			if(i++ != 0)
				if(i > *pRecNo) {
					pRec->SetBuf(sstrlen(line_buf), line_buf);
					(*pRecNo) = i;
					return 1;
				}
	}
	return -1;
}
//
// BhtProtocol
//
#define EOT 0x04 // End Of Transmission
#define ENQ 0x05 // Enquiry
#define ACK 0x06 // Acknowledge
#define NAK 0x15 // Negative Acknowledge

#define SOH 0x01 // Start Of Heading
#define STX 0x02 // Start Of Text
#define ETX 0x03 // End Of Text

SLAPI BhtProtocol::BhtProtocol() : Timeout(3000), MaxTries(10), Flags(0)
{
}

SLAPI BhtProtocol::~BhtProtocol()
{
}

void SLAPI BhtProtocol::SetProtParams(uint timeout, uint maxTries, long flags)
{
	Timeout = timeout;
	MaxTries = maxTries;
	Flags = flags;
}

int SLAPI BhtProtocol::SetConnection()
{
	int    ok = 0, c = 0;
	for(uint i = 0; !ok && i < MaxTries; i++) {
		PutChr(ENQ);
		if((c = GetChr()) == ACK)
			ok = 1;
		else
			SDelay(Timeout);
	}
	if(!ok) {
		PutChr(EOT);
		char   addmsg[32];
		PPSetError(PPERR_BHT_NOHANDSHAKEACK, itoa(c, addmsg, 10));
	}
	return ok;
}

int SLAPI BhtProtocol::SendBlock(uint recNo, size_t size, const char * pBlk)
{
	int    ok = 0;
	char   buf[512], bcc = 0;
	size_t i, p = 0;
	int    enq = 0, nak = 0, j;
	if(recNo > 0) {
		buf[p++] = STX;
		PutIntToBuf(recNo, RECNOLEN, buf+p); p += RECNOLEN;
	}
	else
		buf[p++] = SOH;
	memcpy(buf+p, pBlk, size);
	p += size;
	buf[p++] = ETX;
	bcc = 0;
	for(i = 1; i < p; i++)
		bcc ^= ((uchar)buf[i]);
	buf[p++] = bcc;

	for(j = 0; ok == 0 && j < MaxTries; j++) {
		nak = 0;
		if(!enq)
			for(i = 0; i < p; i++)
				THROW_SL(PutChr(buf[i]));
		switch(GetChr()) {
			case ACK: ok =  1; enq = 0; break;
			case NAK: ok =  0; nak = 1; break;
			case EOT: ok = (PPSetError(PPERR_BHT_EOT), -1); break;
			default:  ok =  0; enq = 1; SDelay(Timeout); break;
		}
	}
	if(enq) {
		PutChr(EOT);
		PPSetError(PPERR_BHT_NOREPLY);
	}
	if(nak) {
		ok = (PPSetError(PPERR_BHT_NAK), -1);
	}
	if(ok < 0)
		ReleaseConnection();
	CATCHZOK
	return ok;
}

int SLAPI BhtProtocol::ReleaseConnection()
{
	int    ok = 0, c;
	for(int i = 0; !ok && i < MaxTries; i++) {
		PutChr(EOT);
		if((c = GetChr()) == ACK)
			ok = 1;
		else
			SDelay(Timeout);
	}
	if(!ok) {
		char   addmsg[32];
		PPSetError(PPERR_BHT_CLOSELINKFAULT, itoa(c, addmsg, 10));
	}
	return ok;
}

int SLAPI BhtProtocol::SendPrgmHeadingText(const char * pFileName, uint numRecs)
{
	char   buf[64];
	size_t p = 0;
	PutStrToBuf(pFileName, FNAMELEN, buf+p); p += FNAMELEN;
	PutIntToBuf(numRecs, NUMRECSLEN, buf+p); p += NUMRECSLEN;
	return SendBlock(0, p, buf);
}

int SLAPI BhtProtocol::SendDataHeadingText(const char * pFileName, uint numRecs, const BhtRecord * pStruc)
{
	char   buf[64];
	size_t p = 0;
	pStruc->GetHeader(pFileName, numRecs, &p, buf);
	return SendBlock(0, p, buf);
}

int SLAPI BhtProtocol::SendPrgmText(uint recNo, size_t blkLen, const char * pBlk)
{
	return SendBlock(recNo, blkLen, pBlk);
}

int SLAPI BhtProtocol::SendRecord(uint recNo, const BhtRecord * pRec)
{
	return SendBlock(recNo, pRec->GetBufLen(), pRec->GetBuf());
}

int SLAPI BhtProtocol::WaitOnConnection(int releaseLink, long timeout)
{
	int    expected_chr = releaseLink ? EOT : ENQ;
	for(uint i = 0; i < 100; i++)
		if(GetChr() == expected_chr) {
			PutChr(ACK);
			return 1;
		}
		else
			SDelay((uint)(timeout / 100));
	return 0;
}

int SLAPI BhtProtocol::ReceiveBlock(uint * pRecNo, size_t * pDataLen, char * pBuf, size_t bufLen)
{
	int    ok = 1;
	uint   recno = 0;
	char   buf[512];
	char   c, bcc = 0;
	size_t p = 0;

	SDelay(100);
	c = GetChr();
	THROW_PP(c == SOH || c == STX, PPERR_BHT_NOTSOHSTXSYMB);
	if(c == STX) {
		char recno_buf[16];
		for(int i = 0; i < RECNOLEN; i++) {
		   	c = GetChr();
			THROW(c);
			recno_buf[i] = c;
			bcc ^= (char)c;
		}
		recno_buf[RECNOLEN] = 0;
		recno = atoi(recno_buf);
		THROW(recno > 0);
	}
	while((c = GetChr()) != ETX && c != 0) {
		buf[p++] = c;
		bcc ^= (char)c;
	}
	THROW(c == ETX);
	bcc ^= (char)c;
	c = GetChr();
	THROW(bcc == c);
	PutChr(ACK);
	ASSIGN_PTR(pRecNo, recno);
	memcpy(pBuf, buf, MIN(p, bufLen));
	ASSIGN_PTR(pDataLen, p);
	CATCH
		ok = 0;
		PutChr(NAK);
	ENDCATCH
	return ok;
}

// static
void SLAPI BhtProtocol::ParseHeadingText(const char * pBuf, char * pFileName, uint * pNumRecs)
{
	char   fname[64];
	long   numrecs = 0;
	GetStrFromBuf(fname, FNAMELEN, pBuf);
	GetIntFromBuf(&numrecs, NUMRECSLEN, pBuf+FNAMELEN);
	strcpy(pFileName, fname);
	*pNumRecs = (uint)numrecs;
}

int SLAPI BhtProtocol::SendPrgmFile(const char * pFileName)
{
	int    ok = 1;
	const  size_t line_size = 256;
	const  size_t blk_size  = 128;
	char   line_buf[line_size];
	uint   numrecs = 0, recno = 0;
	char   fname[MAXPATH], nam[MAXFILE], ext[MAXEXT];
	FILE * stream = 0;

	THROW_PP_S(stream = fopen(pFileName, "r"), PPERR_CANTOPENFILE, pFileName);
	while(fgets(line_buf, sizeof(line_buf), stream))
		numrecs++;
   	THROW(SetConnection());
	fnsplit(pFileName, 0, 0, nam, ext);
	fnmerge(fname, 0, 0, nam, ext);
	THROW(SendPrgmHeadingText(strupr(fname), numrecs) > 0);
	rewind(stream);
	while(fgets(line_buf, sizeof(line_buf), stream)) {
		size_t l = sstrlen(chomp(line_buf));
		if(l < blk_size)
			padright(line_buf, '0', blk_size-l);
		recno++;
		THROW(SendPrgmText(recno, blk_size, line_buf) > 0);
		PPWaitPercent(recno, numrecs);
	}
	THROW(ReleaseConnection());
	CATCHZOK
	SFile::ZClose(&stream);
	return ok;
}

int SLAPI BhtProtocol::SendDataFile(const char * pFileName, const BhtRecord * pStruc)
{
	int    ok = 1;
	const  size_t line_size = 256;
	char   line_buf[line_size];
	uint   numrecs = 0, recno = 0;
	char   fname[MAXPATH], nam[MAXFILE], ext[MAXEXT];
	FILE * stream = 0;
	BhtRecord * p_rec = 0;

	THROW_PP_S(stream = fopen(pFileName, "r"), PPERR_CANTOPENFILE, pFileName);
	THROW_MEM(p_rec = new BhtRecord(*pStruc));
	while(fgets(line_buf, sizeof(line_buf), stream))
		numrecs++;
	THROW_PP(numrecs <= MAXBHTRECS - 1, PPERR_BHTMAXRECSREACHED);
	THROW(SetConnection());
	fnsplit(pFileName, 0, 0, nam, ext);
	fnmerge(fname, 0, 0, nam, ext);
	THROW(SendDataHeadingText(strupr(fname), numrecs, pStruc) > 0);
	rewind(stream);
	while(fgets(line_buf, sizeof(line_buf), stream)) {
		recno++;
		chomp(line_buf);
		p_rec->SetBuf(sstrlen(line_buf), line_buf);
		THROW(SendRecord(recno, p_rec) > 0);
		PPWaitPercent(recno, numrecs);
	}
	THROW(ReleaseConnection());
	CATCHZOK
	delete p_rec;
	SFile::ZClose(&stream);
	return ok;
}

static int FASTCALL _StoreDataBlock(size_t dataLen, const char * pBuf, FILE * out)
{
	if(out) {
		for(size_t i = 0; i < dataLen; i++)
			fputc(pBuf[i], out);
		fputc('\n', out);
	}
	return 1;
}

int SLAPI BhtProtocol::ReceiveFile(const char * pFileName, long timeout)
{
	int    ok = 1;
	FILE * out = fopen(pFileName, "w");
	if(WaitOnConnection(0, timeout)) {
		uint   numrecs = 0, recno = 0, j;
		char   buf[512], fname[32];
		size_t datalen = 0;
		if(ReceiveBlock(&recno, &datalen, buf, sizeof(buf)) > 0) {
			_StoreDataBlock(datalen, buf, out);
			ParseHeadingText(buf, fname, &numrecs);
			for(j = 0; j < numrecs; j++) {
				THROW(ReceiveBlock(&recno, &datalen, buf, sizeof(buf)));
				_StoreDataBlock(datalen, buf, out);
				PPWaitPercent(j, numrecs);
			}
			THROW(WaitOnConnection(1, 1000L));
		}
	}
	else
		ok = -1;
	CATCHZOK
	SFile::ZClose(&out);
	if(ok == 0)
		SFile::Remove(pFileName);
	return ok;
}
//
// CipherProtocol
//
#define EOS  0x0d      // End of string

SLAPI CipherProtocol::CipherProtocol() : Timeout(3000), MaxTries(10), Flags(0)
{
}

SLAPI CipherProtocol::~CipherProtocol()
{
}

void SLAPI CipherProtocol::SetProtParams(uint timeout, uint maxTries, long flags)
{
	Timeout = timeout;
	MaxTries = maxTries;
	Flags = flags;
}

int SLAPI CipherProtocol::SetConnection()
{
	int    ok = 0, c = 0;
	for(uint i = 0; !ok && i < MaxTries; i++) {
		PutChrEx(ENQ);
		if((c = GetChrEx()) == ACK)
			ok = 1;
		else
			SDelay(Timeout);
	}
	if(!ok) {
		PutChrEx(EOT);
		char   addmsg[32];
		PPSetError(PPERR_BHT_NOHANDSHAKEACK, itoa(c, addmsg, 10));
	}
	return ok;
}

int SLAPI CipherProtocol::ReleaseConnection()
{
	int    ok = 0, c;
	for(int i = 0; !ok && i < MaxTries; i++) {
		PutChrEx(EOT);
		if((c = GetChrEx()) == ACK)
			ok = 1;
		else
			SDelay(Timeout);
	}
	if(!ok) {
		char   addmsg[32];
		PPSetError(PPERR_BHT_CLOSELINKFAULT, itoa(c, addmsg, 10));
	}
	return ok;
}

int SLAPI CipherProtocol::SendDataHeadingText(const char * pFileName, uint numRecs, const BhtRecord * pStruc)
{
	char   buf[64];
	size_t p = 0;
	pStruc->GetHeader(pFileName, numRecs, &p, buf);
	return SendBlock(0, p, buf);
}

int SLAPI CipherProtocol::SendDataFile(const char * pFileName, const BhtRecord * pStruc)
{
	int    ok = 1;
	const  size_t line_size = 256;
	char   line_buf[line_size];
	uint   numrecs = 0, recno = 0;
	char   fname[MAXPATH], nam[MAXFILE], ext[MAXEXT];
	FILE * stream = 0;
	BhtRecord * p_rec = 0;

	THROW_PP_S(stream = fopen(pFileName, "r"), PPERR_CANTOPENFILE, pFileName);
	THROW_MEM(p_rec = new BhtRecord(*pStruc));
	while(fgets(line_buf, sizeof(line_buf), stream))
		numrecs++;
   	THROW(SetConnection());
	fnsplit(pFileName, 0, 0, nam, ext);
	fnmerge(fname, 0, 0, nam, ext);
	THROW(SendDataHeadingText(strupr(fname), numrecs, pStruc) > 0);
	rewind(stream);
	while(fgets(line_buf, sizeof(line_buf), stream)) {
		recno++;
		chomp(line_buf);
		p_rec->SetBuf(sstrlen(line_buf), line_buf);
		THROW(SendRecord(recno, p_rec) > 0);
		PPWaitPercent(recno, numrecs);
	}
	THROW(ReleaseConnection());
	CATCHZOK
	delete p_rec;
	SFile::ZClose(&stream);
	return ok;
}

int SLAPI CipherProtocol::SendRecord(uint recNo, const BhtRecord * pRec)
{
	return SendBlock(recNo, pRec->GetBufLen(), pRec->GetBuf());
}

int SLAPI CipherProtocol::SendBlock(uint recNo, size_t size, const char * pBlk)
{
	int ok = 0;

	char buf[512];
	uint sum = 0;
	size_t i, p = 0;
	int enq = 0, nak = 0, j;
	if(recNo > 0) {
		buf[p++] = STX;
		PutIntToBuf(recNo, RECNOLEN, buf+p); p += RECNOLEN;
	}
	else
		buf[p++] = SOH;
	for(i = 0; i < size; i++)
		buf[p++] = pBlk[i];
	for(i = 1; i < p; i++)
		sum += (uint)(uchar)buf[i];
	buf[p++] = (sum / 256 == EOS) ? 14 : sum / 256;
	buf[p++] = (sum % 256 == EOS) ? 14 : sum % 256;
	buf[p++] = EOS;
	for(j = 0; ok == 0 && j < MaxTries; j++) {
		nak = 0;
		if(!enq)
			for(i = 0; i < p; i++)
				THROW_SL(PutChr(buf[i]));
		switch(GetChrEx()) {
			case ACK: ok =  1; enq = 0; break;
			case NAK: ok =  0; nak = 1; break;
			case EOT: ok = (PPSetError(PPERR_BHT_EOT), -1); break;
			default:  ok =  0; enq = 1; SDelay(Timeout); break;
		}
	}
	if(enq) {
		PutChrEx(EOT);
		PPSetError(PPERR_BHT_NOREPLY);
	}
	if(nak) {
		ok = (PPSetError(PPERR_BHT_NAK), -1);
	}
	/*
	if(ok < 0)
		ReleaseConnection();
	*/
	CATCHZOK
	return ok;
}

// static
void SLAPI CipherProtocol::ParseHeadingText(const char * pBuf, char * pFileName, uint * pNumRecs)
{
	char   fname[64];
	long   numrecs = 0;
	GetStrFromBuf(fname, FNAMELEN, pBuf);
	GetIntFromBuf(&numrecs, NUMRECSLEN, pBuf+FNAMELEN);
	strcpy(pFileName, fname);
	*pNumRecs = (uint)numrecs;
}

int SLAPI CipherProtocol::WaitOnConnection(int releaseLink, long timeout)
{
	int expected_chr = releaseLink ? EOT : ENQ;
	for(uint i = 0; i < 100; i++)
		if(GetChrEx() == expected_chr) {
			PutChrEx(ACK);
			return 1;
		}
		else {
			PutChrEx(NAK);
			SDelay((uint)(timeout / 100));
		}
	return 0;
}

int SLAPI CipherProtocol::ReceiveFile(const char * pFileName, long timeout)
{
	int    ok = 1, r = 0;
	FILE * out = fopen(pFileName, "w");
	if(WaitOnConnection(0, timeout)) {
		uint   numrecs = 0, recno = 0, j;
		char   buf[512], fname[32];
		size_t datalen = 0;
		THROW(r = ReceiveBlock(&recno, &datalen, buf, sizeof(buf)));
		if(r > 0) {
			_StoreDataBlock(datalen, buf, out);
			ParseHeadingText(buf, fname, &numrecs);
			for(j = 0; j < numrecs; j++) {
				THROW(ReceiveBlock(&recno, &datalen, buf, sizeof(buf)));
				_StoreDataBlock(datalen, buf, out);
				PPWaitPercent(j+1, numrecs);
			}
			THROW(WaitOnConnection(1, 1000L));
		}
	}
	else
		ok = -1;
	CATCHZOK
	SFile::ZClose(&out);
	if(ok == 0)
		SFile::Remove(pFileName);
	return ok;
}

int SLAPI CipherProtocol::ReceiveBlock(uint * pRecNo, size_t * pDataLen, char * pBuf, size_t bufLen)
{
	int  ok = 1, r = 0;
	uint recno = 0, sum = 0;
	char buf[512];
	char c;
	size_t p = 0;

	for(int i = 0; !r && i < 100; i++) {
		SDelay(300);
		c = GetChr();
		if(c == SOH || c == STX) {
			if(c == STX) {
				char recno_buf[16];
				for(int i = 0; i < RECNOLEN; i++) {
		   			c = GetChr();
					THROW(c);
					recno_buf[i] = c;
					sum += (uint)(uchar)c;
				}
				recno_buf[RECNOLEN] = 0;
				recno = atoi(recno_buf);
				THROW(recno > 0);
			}
			c = GetChr();
			while(c != 0 && c != EOS) {
				buf[p++] = c;
				sum += (uint)(uchar)c;
				c = GetChr();
			}
			sum -= (uint)(uchar)buf[p-2] + (uint)(uchar)buf[p-1];
			THROW((uint)(uchar)buf[p-2] == ((sum / 256 == EOS) ? 14 : sum / 256));
			THROW((uint)(uchar)buf[p-1] == ((sum % 256 == EOS) ? 14 : sum % 256));
			buf[p-2] = '\0';
			p -= 2;
			PutChrEx(ACK);
			ASSIGN_PTR(pRecNo, recno);
			memcpy(pBuf, buf, MIN(p, bufLen));
			ASSIGN_PTR(pDataLen, p);
			r = 1;
		}
		else {
			PutChrEx(NAK);
			PPSetError(PPERR_BHT_NOTSOHSTXSYMB);
		}
	}
	THROW(r);
	CATCH
		ok = 0;
		PutChrEx(EOT);
	ENDCATCH
	return ok;
}

int SLAPI CipherProtocol::PutChrEx(int c)
{
	int    ok = 1;
	THROW(PutChr(c));
	THROW(PutChr(EOS));
	CATCHZOK
	return ok;
}

int SLAPI CipherProtocol::GetChrEx()
{
	int c = GetChr();
	if(c != 0)
		GetChr();
	return c;
}

//static
int SLAPI PPObjBHT::TransmitProgram()
{
	int    ok = 1, valid_data = 0;
	SString path;
	PPID   bht_id = 0;
	PPObjBHT bht_obj;

	if(bht_obj.EnumItems(&bht_id, 0) <= 0)
		return -1;

	TDialog * dlg = new TDialog(DLG_BHTPROGUPLOAD);
	THROW(CheckDialogPtr(&dlg));
	PPGetFilePath(PPPATH_IN, "STYLOBHT.PD3", path);
	bht_id = bht_obj.GetSingle();
	SetupPPObjCombo(dlg, CTLSEL_BHTSEND_BHT, PPOBJ_BHT, bht_id, 0);
	FileBrowseCtrlGroup::Setup(dlg, CTLBRW_BHTSEND_FILENAME, CTL_BHTSEND_FILENAME, 1, 0, 0, FileBrowseCtrlGroup::fbcgfFile);
	dlg->setCtrlString(CTL_BHTSEND_FILENAME, path);
	for(valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		PPBhtTerminal bht_rec;
		dlg->getCtrlData(CTLSEL_BHTSEND_BHT, &bht_id);
		dlg->getCtrlString(CTL_BHTSEND_FILENAME, path);
		if(bht_obj.Search(bht_id, &bht_rec) > 0) {
			if(bht_rec.BhtTypeID != PPObjBHT::btDenso)
				PPError(PPERR_BHTLDPGMNOTSUPPR, 0);
			else if(!fileExists(path))
				PPError(PPERR_NOSRCFILE, path);
			else
				valid_data = 1;
		}
		else
			PPError();
	}
	delete dlg;
	dlg = 0;
	if(valid_data) {
		BhtProtocol bp;
		THROW(bht_obj.InitProtocol(bht_id, &bp));
		PPWait(1);
		THROW(bp.SendPrgmFile(path));
		PPWait(0);
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
int SLAPI PPObjBHT::InitProtocol(PPID id, BhtProtocol * pProt)
{
	int    ok = 1;
	int    com_port = -1;
	char   msg_buf[32];
	PPBhtTerminal rec;
	CommPortParams cpp;
	CommPortTimeouts cpt;

	THROW(Search(id, &rec) > 0);

	pProt->GetParams(&cpp);
	cpp.Cbr = rec.Cbr;
	pProt->SetParams(&cpp);

	pProt->GetTimeouts(&cpt);
	cpt.W_Get_Delay = (rec.ComGet_Delay > 0 && rec.ComGet_Delay <= 1000) ? rec.ComGet_Delay : 75;
	pProt->SetTimeouts(&cpt);
	pProt->SetProtParams(rec.BhtpTimeout, rec.BhtpMaxTries, 0);
	{
		int    rcc = 0;
		int    rcd = 0;
		SString read_cycling_param;
		SString cycle_count, cycle_delay;
		PPIniFile ini_file;
		if(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_COMPORTREADCYCLING, read_cycling_param) > 0) {
			read_cycling_param.Divide(',', cycle_count, cycle_delay);
			rcc = cycle_count.ToLong();
			rcd = cycle_delay.ToLong();
		}
		if(rcc < 0 || rcc > 1000)
			rcc = 10;
		if(rcd < 0 || rcd > 5000)
			rcd = 10;
		pProt->SetReadCyclingParams(rcc, rcd);
	}
	if(IsComDvcSymb(rec.Port, &com_port) == comdvcsCom)
		if(com_port > 0)
			com_port--;
	THROW_PP_S(pProt->InitPort(com_port), PPERR_SLIB, itoa(com_port, msg_buf, 10));
	CATCHZOK
	return ok;
}

int SLAPI PPObjBHT::InitProtocol(PPID id, CipherProtocol * pProt)
{
	int    ok = 1;
	int    com_port = -1;
	char   msg_buf[32];
	PPBhtTerminal rec;
	CommPortParams cpp;
	CommPortTimeouts cpt;

	THROW(Search(id, &rec) > 0);

	pProt->GetParams(&cpp);
	cpp.Cbr = rec.Cbr;
	pProt->SetParams(&cpp);

	pProt->GetTimeouts(&cpt);
	if(rec.ComGet_Delay == 0 || rec.ComGet_Delay > 1000)
		cpt.W_Get_Delay = 200; /* default 150 */
	else
		cpt.W_Get_Delay = rec.ComGet_Delay;
	pProt->SetTimeouts(&cpt);
	pProt->SetProtParams(rec.BhtpTimeout, rec.BhtpMaxTries, 0);
	if(IsComDvcSymb(rec.Port, &com_port) == comdvcsCom)
		if(com_port > 0)
			com_port--;
	THROW_PP_S(pProt->InitPort(com_port), PPERR_SLIB, itoa(com_port, msg_buf, 10));
	CATCHZOK
	return ok;
}

void SLAPI PPObjBHT::InitGoodsBhtRec(BhtRecord * pBhtRec) const
{
	pBhtRec->Reset();
	pBhtRec->AddFld(8);  // GoodsID
	pBhtRec->AddFld(14); // Barcode
	// @v4.0.8 pBhtRec->AddFld(6);  // Price
}

void SLAPI PPObjBHT::InitSupplBhtRec(BhtRecord * pBhtRec) const
{
	pBhtRec->Reset();
	pBhtRec->AddFld(8);  // Suppl ID (ar_rec.ID [not ar_rec.Article])
	pBhtRec->AddFld(30); // Name
}

static int SLAPI PutBhtRecToFile(const BhtRecord * pBhtRec, FILE * stream)
{
	size_t buf_len = pBhtRec->GetBufLen();
	const  char * p_buf = pBhtRec->GetBuf();
	for(size_t i = 0; i < buf_len; i++)
		fputc(p_buf[i], stream);
	fputc('\n', stream);
	return 1;
}

// AHTOXA {
int SLAPI PPObjBHT::PrepareTechSessData(const char * pPath, PPID bhtTypeID)
{
	int    ok = 1;
	long   n = 0;
	DbfTable * p_dbf_tbl = 0; // AHTOXA
	TSessionFilt filt;
	TSessionViewItem item;
	ProcessorTbl::Rec prc_rec;
	PPObjProcessor prc_obj;
	PPViewTSession view;

	PPSetAddedMsgString(pPath);
	PPWait(1);
	if(bhtTypeID == PPObjBHT::btPalm) {
		int    num_flds = 0;
		DBFCreateFld fld_list[32];
		THROW(p_dbf_tbl = new DbfTable(pPath));
		fld_list[num_flds++].Init("ID",     'N', 10, 0);
		fld_list[num_flds++].Init("NUMBER", 'N', 10, 0);
		fld_list[num_flds++].Init("NAME",   'C', 48, 0);
		fld_list[num_flds++].Init("DATE",   'D',  8, 0);
		fld_list[num_flds++].Init("Tm",     'N', 10, 0);
		THROW(p_dbf_tbl->create(num_flds, fld_list));
		THROW(p_dbf_tbl->open());
	}
	filt.StatusFlags = (1 << TSESST_PENDING) | (1 << TSESST_INPROCESS);
	THROW(view.Init_(&filt));
	for(view.InitIteration(0); view.NextIteration(&item) > 0;)  {
		if(!(item.Flags & TSESF_SUPERSESS)) {
			if(bhtTypeID == PPObjBHT::btPalm && prc_obj.Search(item.PrcID, &prc_rec) > 0) {
				DbfRecord dbf_rec(p_dbf_tbl);
				dbf_rec.put(1, item.ID);
				dbf_rec.put(2, item.Num);
				dbf_rec.put(3, prc_rec.Name);
				dbf_rec.put(4, item.StDt);
				dbf_rec.put(5, (long)item.StTm.v);
				THROW(p_dbf_tbl->appendRec(&dbf_rec));
			}
		}
	}
	CATCHZOK
	delete p_dbf_tbl;
	return ok;
}

int SLAPI PPObjBHT::PrepareLocData(const char * pPath, PPID bhtTypeID)
{
	int    ok = 1;
	LocationTbl::Rec l_rec;
	DbfTable * p_dbf_tbl = 0;
	PPObjLocation loc_obj;
	PPIDArray wh_list;

	loc_obj.GetWarehouseList(&wh_list);
	PPSetAddedMsgString(pPath);
	PPWait(1);
	if(bhtTypeID == PPObjBHT::btPalm) {
		int    num_flds = 0;
		DBFCreateFld fld_list[32];
		THROW(p_dbf_tbl = new DbfTable(pPath));
		fld_list[num_flds++].Init("ID",   'N', 10, 0);
		fld_list[num_flds++].Init("NAME", 'C', 50, 0);
		THROW(p_dbf_tbl->create(num_flds, fld_list));
		THROW(p_dbf_tbl->open());
	}
	for(uint i = 0; i < wh_list.getCount(); i++)
		if(loc_obj.Search(wh_list.at(i), &l_rec) > 0 && bhtTypeID == PPObjBHT::btPalm) {
			DbfRecord dbf_rec(p_dbf_tbl);
			dbf_rec.put(1, l_rec.ID);
			dbf_rec.put(2, strlwr866(l_rec.Name));
			THROW(p_dbf_tbl->appendRec(&dbf_rec));
		}
	CATCHZOK
	delete p_dbf_tbl;
	return ok;
}
// } AHTOXA

int SLAPI PPObjBHT::PrepareBillRowCellData(PPBhtTerminalPacket * pPack, PPID billID)
{
	int    ok = -1, num_flds = 0;
	SString fname, path, serial;
	SString temp_buf;
	PPBillPacket pack;
	PPImpExp * p_ie_brow = 0;
	if(P_BObj->ExtractPacketWithFlags(billID, &pack, BPLD_FORCESERIALS) > 0) {
		uint i = 0;
		PPTransferItem ti;
		PPImpExpParam ie_param_brow;

		PPGetFileName(PPFILNAM_BHT_BROWSWCELLS, fname);
		(path = pPack->ImpExpPath_).SetLastSlash().Cat(fname);
		THROW(InitImpExpDbfParam(PPREC_SBIIBILLROWWITHCELLS, &ie_param_brow, path, 1));
		THROW_MEM(p_ie_brow = new PPImpExp(&ie_param_brow, 0));
		THROW(p_ie_brow->OpenFileForWriting(0, 1));

		for(pack.InitExtTIter(0); pack.EnumTItemsExt(0, &ti) > 0; i++) {
			double qtty = 0.0;
			Sdr_SBIIBillRowWithCells sdr_brow;

			MEMSZERO(sdr_brow);
			// @v9.8.11 pack.SnL.GetNumber(i, &serial);
			pack.LTagL.GetNumber(PPTAG_LOT_SN, i, serial); // @v9.8.11 
			sdr_brow.BillID   = billID;
			sdr_brow.GoodsID  = ti.GoodsID;
			serial.CopyTo(sdr_brow.Serial, sizeof(sdr_brow.Serial));
	 		sdr_brow.Qtty     = fabs(ti.Qtty());
			sdr_brow.Cost     = ti.Cost;
			sdr_brow.RByBill  = (long)ti.RByBill;
			GetObjectName(PPOBJ_GOODS, ti.GoodsID, sdr_brow.Name, sizeof(sdr_brow.Name));
			// @v9.4.11 SOemToChar(sdr_brow.Name);
			(temp_buf = sdr_brow.Name).Transf(CTRANSF_INNER_TO_OUTER); // @v9.4.11
			STRNSCPY(sdr_brow.Name, temp_buf); // @v9.4.11
			qtty = sdr_brow.Qtty;
			{
				LocTransfCore loct_tbl;
				TSVector <LocTransfTbl::Rec> cell_list; // @v9.8.4 TSArray-->TSVector
				if(IsExpendOp(pack.Rec.OpID)) {
					RAssocArray list;
					SString name;
					//
					// ��������� ������� ������� ������ ��� ���������
					//
					{
						LocTransfTbl::Rec loct_rec;
						uint   j;
						MEMSZERO(loct_rec);
						qtty = sdr_brow.Qtty;
						loct_tbl.GetTransByBill(billID, ti.RByBill, &cell_list);
						for(j = 0; j < cell_list.getCount(); j++)
							sdr_brow.Qtty -= fabs(cell_list.at(j).Qtty);
						sdr_brow.Expended = BIN(qtty != sdr_brow.Qtty);
						qtty = sdr_brow.Qtty;
						THROW(p_ie_brow->AppendRecord(&sdr_brow, sizeof(sdr_brow)));
						sdr_brow.Expended = 1;
						for(j = 0; j < cell_list.getCount(); j++) {
							sdr_brow.LocID = cell_list.at(j).LocID;
							sdr_brow.Qtty  = -fabs(cell_list.at(j).Qtty);
							GetObjectName(PPOBJ_LOCATION, sdr_brow.LocID, sdr_brow.Name, sizeof(sdr_brow.Name));
							name.Z().Space().Space().Space().Space().Cat(sdr_brow.Name);
							name.CopyTo(sdr_brow.Name, sizeof(sdr_brow.Name));
							// @v9.4.11 SOemToChar(sdr_brow.Name);
							(temp_buf = sdr_brow.Name).Transf(CTRANSF_INNER_TO_OUTER); // @v9.4.11
							STRNSCPY(sdr_brow.Name, temp_buf); // @v9.4.11
							THROW(p_ie_brow->AppendRecord(&sdr_brow, sizeof(sdr_brow)));
						}
					}
					//
					// ��������� ������, � ������� ������������ ������ �����
					// ������ ��� ������� ������� ��� �� ��������� ���������
					//
					if(qtty > 0) {
						sdr_brow.Expended = 0;
						loct_tbl.GetLocCellList(ti.GoodsID, LConfig.Location, &list);
						for(uint j = 0; j < list.getCount(); j++) {
							sdr_brow.LocID = list.at(j).Key;
							sdr_brow.Qtty  = list.at(j).Val;
							GetObjectName(PPOBJ_LOCATION, sdr_brow.LocID, sdr_brow.Name, sizeof(sdr_brow.Name));
							name.Z().Space().Space().Space().Space().Cat(sdr_brow.Name);
							name.CopyTo(sdr_brow.Name, sizeof(sdr_brow.Name));
							// @v9.4.11 SOemToChar(sdr_brow.Name);
							(temp_buf = sdr_brow.Name).Transf(CTRANSF_INNER_TO_OUTER); // @v9.4.11
							STRNSCPY(sdr_brow.Name, temp_buf); // @v9.4.11
							THROW(p_ie_brow->AppendRecord(&sdr_brow, sizeof(sdr_brow)));
						}
					}
				}
				else {
					loct_tbl.GetDisposition(billID, ti.RByBill, cell_list);
					for(uint j = 0; j < cell_list.getCount(); j++)
						qtty -= cell_list.at(j).Qtty;
					sdr_brow.Expended = BIN(sdr_brow.Qtty != qtty);
					sdr_brow.Qtty = qtty;
					THROW(p_ie_brow->AppendRecord(&sdr_brow, sizeof(sdr_brow)));
					sdr_brow.Expended = 1;
					for(uint j = 0; j < cell_list.getCount(); j++) {
						SString name;
						sdr_brow.Qtty  = -cell_list.at(j).Qtty;
						sdr_brow.LocID = cell_list.at(j).LocID;
						GetObjectName(PPOBJ_LOCATION, sdr_brow.LocID, sdr_brow.Name, sizeof(sdr_brow.Name));
						name.Space().Space().Space().Space().Cat(sdr_brow.Name);
						name.CopyTo(sdr_brow.Name, sizeof(sdr_brow.Name));
						// @v9.4.11 SOemToChar(sdr_brow.Name);
						(temp_buf = sdr_brow.Name).Transf(CTRANSF_INNER_TO_OUTER); // @v9.4.11
						STRNSCPY(sdr_brow.Name, temp_buf); // @v9.4.11
						THROW(p_ie_brow->AppendRecord(&sdr_brow, sizeof(sdr_brow)));
					}
				}
			}
		}
	}
	CATCHZOK
	ZDELETE(p_ie_brow);
	return ok;
}

#if 0 // @v9.4.9 {

int SLAPI PPObjBHT::PrepareBillData(PPBhtTerminalPacket * pPack, int uniteGoods /*=1*/)
{
	int    ok = -1, num_flds = 0;
	SString fname, path, h_path, r_path, serial;
	PPIDArray draft_op_list, goods_op_list, order_op_list;
	PPImpExp * p_ie_bill = 0, * p_ie_brow = 0;

	THROW_INVARG(pPack);
	THROW_PP(pPack->Rec.BhtTypeID == PPObjBHT::btStyloBhtII && pPack->P_SBIICfg, PPERR_SBII_CFGNOTVALID);
	PPWait(1);
	for(uint i = 0; i < pPack->P_SBIICfg->P_OpList->getCount(); i++) {
		PPID op_id = pPack->P_SBIICfg->P_OpList->at(i).OpID;
		if(op_id > 0) {
			if(IsDraftOp(op_id))
				draft_op_list.add(op_id);
			else if(GetOpType(op_id) == PPOPT_GOODSORDER)
				order_op_list.add(op_id);
			else
				goods_op_list.add(op_id);
		}
	}
	draft_op_list.sort();
	goods_op_list.sort();
	order_op_list.sort();
	if(draft_op_list.getCount() || goods_op_list.getCount() || order_op_list.getCount()) {
		PPIDArray bbt_list;
		//
		// ����� ����������
		//
		{
			PPImpExpParam ie_param_bill;
			PPGetFilePath(PPPATH_OUT, PPFILNAM_BHT_SAMPLEBILLS, h_path);
			THROW(InitImpExpDbfParam(PPREC_SBIISAMPLEBILL, &ie_param_bill, h_path, 1));
			THROW_MEM(p_ie_bill = new PPImpExp(&ie_param_bill, 0));
			THROW(p_ie_bill->OpenFileForWriting(0, 1));
		}
		//
		// ������ ����������
		//
		{
			PPImpExpParam ie_param_brow;
			PPGetFilePath(PPPATH_OUT, PPFILNAM_BHT_SAMPLEBROWS, r_path);
			THROW(InitImpExpDbfParam(PPREC_SBIISAMPLEBILLROW, &ie_param_brow, r_path, 1));
			THROW_MEM(p_ie_brow = new PPImpExp(&ie_param_brow, 0));
			THROW(p_ie_brow->OpenFileForWriting(0, 1));
		}
		bbt_list.add(bbtGoodsBills);
		bbt_list.add(bbtOrderBills);
		bbt_list.add(bbtDraftBills);
		for(uint i = 0; i < bbt_list.getCount(); i++) {
			PPIDArray * p_op_list = 0;
			BrowseBillsType bbt = (BrowseBillsType)bbt_list.at(i);
			if(bbt == bbtDraftBills)
				p_op_list = &draft_op_list;
			else if(bbt == bbtOrderBills)
				p_op_list = &order_op_list;
			else
				p_op_list = &goods_op_list;
			if(p_op_list && p_op_list->getCount()) {
				BillViewItem item;
				BillFilt   bill_filt;
				PPViewBill v_bill;
				bill_filt.SetupBrowseBillsType(bbt);
				if(bbt == bbtOrderBills)
					bill_filt.DuePeriod = pPack->P_SBIICfg->ExportBillsPeriod;
				else
					bill_filt.Period = pPack->P_SBIICfg->ExportBillsPeriod;
				bill_filt.DuePeriod.Actualize(ZERODATE);
				bill_filt.Period.Actualize(ZERODATE);
				THROW(v_bill.Init_(&bill_filt));
				for(v_bill.InitIteration(PPViewBill::OrdByDate); v_bill.NextIteration(&item) > 0; PPWaitPercent(v_bill.GetCounter())) {
					if(p_op_list->bsearch(item.OpID, 0) > 0) {
						int    fld_num = 1;
						QuotIdent suppl_deal_qi(item.Dt, item.LocID, 0, 0, item.Object);
						PPBillPacket pack;
						Sdr_SBIISampleBill sdr_bill;

						MEMSZERO(sdr_bill);
						SOemToChar(item.Code);
						sdr_bill.ID      = item.ID;
						sdr_bill.Date    = item.Dt;
						sdr_bill.Article = item.Object;
						sdr_bill.OpID    = item.OpID;
						STRNSCPY(sdr_bill.Code, item.Code);
						THROW(p_ie_bill->AppendRecord(&sdr_bill, sizeof(sdr_bill)));
						if(P_BObj->ExtractPacket(item.ID, &pack, BPLD_FORCESERIALS) > 0) {
							uint   i = 0;
							PPTransferItem ti;
							for(pack.InitExtTIter((uniteGoods) ? ETIEF_UNITEBYGOODS : 0); pack.EnumTItemsExt(0, &ti) > 0; i++) {
								Sdr_SBIISampleBillRow sdr_brow;
   								MEMSZERO(sdr_brow);
								pack.SnL.GetNumber(i, &serial);
								sdr_brow.BillID  = item.ID;
								sdr_brow.GoodsID = ti.GoodsID;
								serial.CopyTo(sdr_brow.Serial, sizeof(sdr_brow.Serial));
	 							sdr_brow.Qtty = fabs(ti.Qtty());
								sdr_brow.Cost = ti.Cost;
								{
									PPSupplDeal sd;
									GObj.GetSupplDeal(ti.GoodsID, suppl_deal_qi, &sd);
									sdr_brow.SuplDeal = sd.Cost;
									sdr_brow.SuplDLow = (int16)(sd.DnDev * 100);
									sdr_brow.SuplDUp  = (int16)(sd.UpDev * 100);
								}
								THROW(p_ie_brow->AppendRecord(&sdr_brow, sizeof(sdr_brow)));
							}
						}
					}
				}
			}
		}
		if(p_ie_bill && p_ie_brow) {
			p_ie_bill->CloseFile();
			p_ie_brow->CloseFile();
			PPGetFileName(PPFILNAM_BHT_SAMPLEBILLS, fname);
			(path = pPack->ImpExpPath_).SetLastSlash().Cat(fname).Transf(CTRANSF_INNER_TO_OUTER);
			SCopyFile(h_path, path, 0, FILE_SHARE_READ, 0);

			PPGetFileName(PPFILNAM_BHT_SAMPLEBROWS, fname);
			(path = pPack->ImpExpPath_).SetLastSlash().Cat(fname).Transf(CTRANSF_INNER_TO_OUTER);
			SCopyFile(r_path, path, 0, FILE_SHARE_READ, 0);
		}
	}
	CATCHZOK
	ZDELETE(p_ie_bill);
	ZDELETE(p_ie_brow);
	return ok;
}

#endif // } 0 @v9.4.9

struct BHT_BillOpEntry {
	BHT_BillOpEntry() : OpID(0), Flags(0), Bbt(bbtUndef)
	{
		Period.SetZero();
		DuePeriod.SetZero();
	}
	enum {
		fUseDueDate = 0x0001
	};
	PPID   OpID;
	long   Flags;
	BrowseBillsType Bbt;
    DateRange Period;
    DateRange DuePeriod;
};

int SLAPI PPObjBHT::PrepareBillData2(PPBhtTerminalPacket * pPack, PPIDArray * pGoodsList, int uniteGoods /*=1*/)
{
	int    ok = -1, num_flds = 0;
	uint   i;
	SString fname, path;
	SString h_path;
	SString r_path;
	SString serial;
	SString temp_buf;
	PPImpExp * p_ie_bill = 0, * p_ie_brow = 0;

	TSArray <BHT_BillOpEntry> bill_op_list;
	PPIDArray result_goods_list;

	THROW_INVARG(pPack);
	THROW_PP(pPack->Rec.BhtTypeID == PPObjBHT::btStyloBhtII && pPack->P_SBIICfg, PPERR_SBII_CFGNOTVALID);
	PPWait(1);
	{
		PPUserFuncProfiler ufp(PPUPRF_BHTPREPBILL); // @v9.4.11
		double prf_measure = 0.0;
		for(i = 0; i < pPack->P_SBIICfg->P_OpList->getCount(); i++) {
			const SBIIOpInfo & r_item = pPack->P_SBIICfg->P_OpList->at(i);
			const PPID op_id = r_item.OpID;
			if(op_id > 0) {
				BHT_BillOpEntry new_entry;
				new_entry.OpID = op_id;
				if(r_item.Flags & StyloBhtIIConfig::foprUseDueDate) {
					new_entry.Flags |= BHT_BillOpEntry::fUseDueDate;
					(new_entry.DuePeriod = pPack->P_SBIICfg->ExportBillsPeriod).Actualize(ZERODATE);
				}
				else {
					(new_entry.Period = pPack->P_SBIICfg->ExportBillsPeriod).Actualize(ZERODATE);
				}
				if(IsDraftOp(op_id)) {
					new_entry.Bbt = bbtDraftBills;
				}
				else if(GetOpType(op_id) == PPOPT_GOODSORDER) {
					new_entry.Bbt = bbtOrderBills;
				}
				else {
					new_entry.Bbt = bbtGoodsBills;
				}
				THROW_SL(bill_op_list.insert(&new_entry));
			}
		}
		if(bill_op_list.getCount()) {
			PPIDArray bbt_list;
			//
			// ����� ����������
			//
			{
				PPImpExpParam ie_param_bill;
				// @v9.4.11 PPGetFilePath(PPPATH_OUT, PPFILNAM_BHT_SAMPLEBILLS, h_path);
				PPMakeTempFileName("bht", "dbf", 0, h_path); // @v9.4.11
				THROW(InitImpExpDbfParam(PPREC_SBIISAMPLEBILL, &ie_param_bill, h_path, 1));
				THROW_MEM(p_ie_bill = new PPImpExp(&ie_param_bill, 0));
				THROW(p_ie_bill->OpenFileForWriting(0, 1));
			}
			//
			// ������ ����������
			//
			{
				PPImpExpParam ie_param_brow;
				// @v9.4.11 PPGetFilePath(PPPATH_OUT, PPFILNAM_BHT_SAMPLEBROWS, r_path);
				PPMakeTempFileName("bht", "dbf", 0, r_path); // @v9.4.11
				THROW(InitImpExpDbfParam(PPREC_SBIISAMPLEBILLROW, &ie_param_brow, r_path, 1));
				THROW_MEM(p_ie_brow = new PPImpExp(&ie_param_brow, 0));
				THROW(p_ie_brow->OpenFileForWriting(0, 1));
			}
			for(i = 0; i < bill_op_list.getCount(); i++) {
				const BHT_BillOpEntry & r_entry = bill_op_list.at(i);
				BillViewItem item;
				BillFilt bill_filt;
				PPViewBill v_bill;
				bill_filt.SetupBrowseBillsType(r_entry.Bbt);
				// @debug {
				/**/
				if(r_entry.Bbt == bbtDraftBills)
					bill_filt.Ft_ClosedOrder = 0;
				/**/
				// } @debug
				bill_filt.OpID = r_entry.OpID;
				if(r_entry.Flags & BHT_BillOpEntry::fUseDueDate) {
					bill_filt.DuePeriod = r_entry.DuePeriod;
				}
				else {
					bill_filt.Period = r_entry.Period;
				}
				if(!bill_filt.Period.low) {
					const LDATE ctrl_pt_dt = NZOR(bill_filt.Period.upp, NZOR(bill_filt.DuePeriod.upp, NZOR(bill_filt.DuePeriod.low, getcurdate_()))); // @v9.4.11
					bill_filt.Period.low = plusdate(ctrl_pt_dt, -30); // @v9.4.11
					/* @v9.4.11 if(bill_filt.Period.upp)
						bill_filt.Period.low = plusdate(bill_filt.Period.upp, -90);
					else
						bill_filt.Period.low = plusdate(getcurdate_(), -90);
					*/
				}
				THROW(v_bill.Init_(&bill_filt));
				for(v_bill.InitIteration(PPViewBill::OrdByDate); v_bill.NextIteration(&item) > 0; PPWaitPercent(v_bill.GetCounter())) {
					int    fld_num = 1;
					const  QuotIdent suppl_deal_qi(item.Dt, item.LocID, 0, 0, item.Object);
					PPBillPacket pack;
					Sdr_SBIISampleBill sdr_bill;

					MEMSZERO(sdr_bill);
					// @v9.4.11 SOemToChar(item.Code);
					(temp_buf = item.Code).Transf(CTRANSF_INNER_TO_OUTER); // @v9.4.11
					STRNSCPY(item.Code, temp_buf); // @v9.4.11
					sdr_bill.ID      = item.ID;
					sdr_bill.Date    = (r_entry.Flags & BHT_BillOpEntry::fUseDueDate && checkdate(item.DueDate)) ? item.DueDate : item.Dt;
					sdr_bill.Article = item.Object;
					sdr_bill.OpID    = item.OpID;
					STRNSCPY(sdr_bill.Code, item.Code);
					THROW(p_ie_bill->AppendRecord(&sdr_bill, sizeof(sdr_bill)));
					if(P_BObj->ExtractPacketWithFlags(item.ID, &pack, BPLD_FORCESERIALS) > 0) {
						uint   i = 0;
						PPTransferItem ti;
						for(pack.InitExtTIter(uniteGoods ? ETIEF_UNITEBYGOODS : 0); pack.EnumTItemsExt(0, &ti) > 0; i++) {
							Sdr_SBIISampleBillRow sdr_brow;
							MEMSZERO(sdr_brow);
							// @v9.8.11 pack.SnL.GetNumber(i, &serial);
							pack.LTagL.GetNumber(PPTAG_LOT_SN, i, serial); // @v9.8.11 
							sdr_brow.BillID  = item.ID;
							sdr_brow.GoodsID = ti.GoodsID;
							serial.CopyTo(sdr_brow.Serial, sizeof(sdr_brow.Serial));
							sdr_brow.Qtty = fabs(ti.Qtty());
							sdr_brow.Cost = ti.Cost;
							{
								PPSupplDeal sd;
								GObj.GetSupplDeal(ti.GoodsID, suppl_deal_qi, &sd, 1); // @v9.5.3 useCache=1
								sdr_brow.SuplDeal = sd.Cost;
								sdr_brow.SuplDLow = (int16)(sd.DnDev * 100);
								sdr_brow.SuplDUp  = (int16)(sd.UpDev * 100);
							}
							THROW(p_ie_brow->AppendRecord(&sdr_brow, sizeof(sdr_brow)));
							result_goods_list.add(labs(ti.GoodsID));
						}
					}
					prf_measure += 1.0; // @v9.4.11
				}
			}
			if(p_ie_bill && p_ie_brow) {
				p_ie_bill->CloseFile();
				p_ie_brow->CloseFile();
				PPGetFileName(PPFILNAM_BHT_SAMPLEBILLS, fname);
				(path = pPack->ImpExpPath_).SetLastSlash().Cat(fname).Transf(CTRANSF_INNER_TO_OUTER);
				SCopyFile(h_path, path, 0, FILE_SHARE_READ, 0);

				PPGetFileName(PPFILNAM_BHT_SAMPLEBROWS, fname);
				(path = pPack->ImpExpPath_).SetLastSlash().Cat(fname).Transf(CTRANSF_INNER_TO_OUTER);
				SCopyFile(r_path, path, 0, FILE_SHARE_READ, 0);
			}
		}
		ufp.SetFactor(0, prf_measure); // @v9.4.11
		ufp.Commit(); // @v9.4.11
	}
	CATCHZOK
	ZDELETE(p_ie_bill);
	ZDELETE(p_ie_brow);
	result_goods_list.sortAndUndup();
	ASSIGN_PTR(pGoodsList, result_goods_list);
	return ok;
}

int SLAPI PPObjBHT::PrepareLocCellData(PPBhtTerminalPacket * pPack)
{
	int    ok = -1;
	SString path;
	SString out_path;
	SString temp_buf;
	PPImpExp * p_ie_loc = 0;
	LocationFilt filt(LOCTYP_WHZONE, 0, LConfig.Location);
	StrAssocArray * p_cell_list = 0;
	PPObjLocation loc_obj;

	THROW_INVARG(pPack);
	THROW_PP(pPack->Rec.BhtTypeID == PPObjBHT::btStyloBhtII && pPack->P_SBIICfg, PPERR_SBII_CFGNOTVALID);
	PPWait(1);
	p_cell_list = loc_obj.MakeList_(&filt);
	if(p_cell_list && p_cell_list->getCount()) {
		PPImpExpParam ie_param;

		PPGetFilePath(PPPATH_OUT, PPFILNAM_BHT_LOCCELLS, out_path);
		/*
		PPGetFileName(PPFILNAM_BHT_LOCCELLS, temp_buf);
		(path = pPack->ImpExpPath).SetLastSlash().Cat(temp_buf);
		*/
		THROW(InitImpExpDbfParam(PPREC_SBIILOCCELL, &ie_param, out_path, 1));
		THROW_MEM(p_ie_loc = new PPImpExp(&ie_param, 0));
		THROW(p_ie_loc->OpenFileForWriting(0, 1));
		if(p_cell_list->getCount()) {
			for(uint i = 0; i < p_cell_list->getCount(); i++) {
				LocationTbl::Rec loc_rec;
				if(loc_obj.Search(p_cell_list->Get(i).Id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WHCELL) {
					Sdr_SBIILocCell sdr_loc;
					MEMSZERO(sdr_loc);
					sdr_loc.ID = loc_rec.ID;
					STRNSCPY(sdr_loc.Code, (temp_buf = loc_rec.Code).Transf(CTRANSF_INNER_TO_OUTER));
					STRNSCPY(sdr_loc.Name, (temp_buf = loc_rec.Name).Transf(CTRANSF_INNER_TO_OUTER));
					sdr_loc.Qtty = 0;
					THROW(p_ie_loc->AppendRecord(&sdr_loc, sizeof(sdr_loc)));
					PPWaitPercent(i + 1, p_cell_list->getCount(), 0);
				}
			}
			p_ie_loc->CloseFile();
			PPGetFileName(PPFILNAM_BHT_LOCCELLS, temp_buf);
			(path = pPack->ImpExpPath_).SetLastSlash().Cat(temp_buf).Transf(CTRANSF_INNER_TO_OUTER);
			SCopyFile(out_path, path, 0, FILE_SHARE_READ, 0);
		}
		else {
			p_ie_loc->CloseFile();
			SFile::Remove(path);
		}
	}
	CATCHZOK
	ZDELETE(p_ie_loc);
	ZDELETE(p_cell_list);
	return ok;
}

int SLAPI PPObjBHT::PrepareConfigData(const PPBhtTerminalPacket * pPack, StyloBhtIIConfig * pCfg)
{
	int    ok = -1, num_flds = 0;
	SString fname, path, out_path;
	SBIIOpInfoArray * p_op_list = 0;
	SFile  file;
	PPImpExpParam ie_param;
	PPImpExp * p_ie_op = 0;
	THROW_INVARG(pPack && pCfg);
	THROW_PP(pPack->Rec.BhtTypeID == PPObjBHT::btStyloBhtII && pPack->P_SBIICfg, PPERR_SBII_CFGNOTVALID);

	PPGetFilePath(PPPATH_OUT, PPFILNAM_BHT_OPLIST, out_path);
	PPWait(1);
	THROW(InitImpExpDbfParam(PPREC_SBIIOPRESTR, &ie_param, out_path, 1));
	THROW_MEM(p_ie_op = new PPImpExp(&ie_param, 0));
	THROW(p_ie_op->OpenFileForWriting(0, 1));
	p_op_list = pPack->P_SBIICfg->P_OpList;
	if(p_op_list) {
		for(uint i = 0; i < p_op_list->getCount(); i++) {
			const SBIIOpInfo & op_info = pPack->P_SBIICfg->P_OpList->at(i);
			long op_id = op_info.ToHostOpID;
			PPOprKind op_data;
			if(op_id && GetOpData(op_id, &op_data) > 0) {
				Sdr_SBIIOpRestr sdr_op;
				sdr_op.OpID     = op_info.OpID;
				sdr_op.AccSheet = op_data.AccSheetID;
				sdr_op.ToBhtOp  = op_info.ToBhtOpID;
				sdr_op.BhtOkCA  = op_info.BhtOkCancelActions;
				sdr_op.BhtCfmA  = op_info.BhtCfmActions;
				THROW(p_ie_op->AppendRecord(&sdr_op, sizeof(sdr_op)));
			}
		}
		p_ie_op->CloseFile();
		PPGetFileName(PPFILNAM_BHT_OPLIST, fname);
		(path = pPack->ImpExpPath_).SetLastSlash().Cat(fname).Transf(CTRANSF_INNER_TO_OUTER);
		SCopyFile(out_path, path, 0, FILE_SHARE_READ, 0);
	}
	PPGetFileName(PPFILNAM_BHT_CONFIG, fname);
	(path = pPack->ImpExpPath_).SetLastSlash().Cat(fname).Transf(CTRANSF_INNER_TO_OUTER);
	pCfg->ToDevice();
	THROW_SL(file.Open(path, SFile::mWrite|SFile::mBinary));
	THROW_SL(file.Write(pCfg, sizeof(StyloBhtIIConfig)));
	CATCHZOK
	ZDELETE(p_ie_op);
	return ok;
}

int SLAPI PPObjBHT::PrepareSupplData(const char * pPath, PPBhtTerminalPacket * pPack /*=0*/)
{
	int    ok = 1;
	long   bht_type_id = (pPack) ? pPack->Rec.BhtTypeID : 0;
	SString out_path;
	FILE * stream = 0;
	BhtRecord * p_bht_rec = 0;
	DbfTable * p_dbf_tbl = 0;

	PPID   acc_sheet_id = GetSupplAccSheet();
	if(acc_sheet_id) {
		long n;
		PPObjArticle ar_obj;
		ArticleTbl::Rec ar_rec;
		BhtProtocol bp;

		PPSetAddedMsgString(pPath);
		PPWait(1);
		if(!oneof3(bht_type_id, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
			THROW_PP(stream = fopen(pPath, "w"), PPERR_CANTOPENFILE);
			THROW_MEM(p_bht_rec = new BhtRecord);
			InitSupplBhtRec(p_bht_rec);
		}
		else {
			int    num_flds = 0, suppl_len = (bht_type_id == PPObjBHT::btWinCe) ? 30 : 50;
			DBFCreateFld fld_list[32];
			SString gname, temp_path;

			PPGetPath(PPPATH_OUT, temp_path);
			SPathStruc::ReplacePath((out_path = pPath), temp_path, 1);

			THROW_MEM(p_dbf_tbl = new DbfTable(out_path));
			fld_list[num_flds++].Init("ID",   'N', 10, 0);
			fld_list[num_flds++].Init("NAME", 'C', suppl_len, 0);
			if(bht_type_id == PPObjBHT::btStyloBhtII)
				fld_list[num_flds++].Init("ACCSHEET", 'C', 50, 0);
			THROW_SL(p_dbf_tbl->create(num_flds, fld_list));
			THROW(p_dbf_tbl->open());
		}
		for(n = 0; ar_obj.P_Tbl->EnumBySheet(acc_sheet_id, &n, &ar_rec) > 0;)  {
			if(!oneof3(bht_type_id, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
				p_bht_rec->PutInt(0, ar_rec.ID);
				p_bht_rec->PutStr(1, strupr866(ar_rec.Name));
				PutBhtRecToFile(p_bht_rec, stream);
			}
			else {
				DbfRecord dbf_rec(p_dbf_tbl);
				dbf_rec.put(1, ar_rec.ID);
				if(oneof2(bht_type_id, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII))
					SOemToChar(ar_rec.Name);
				else
					strlwr866(ar_rec.Name);
				dbf_rec.put(2, ar_rec.Name);
				if(bht_type_id == PPObjBHT::btStyloBhtII)
					dbf_rec.put(3, acc_sheet_id);
				THROW(p_dbf_tbl->appendRec(&dbf_rec));
			}
		}
		if(bht_type_id == PPObjBHT::btStyloBhtII) {
			uint count = (pPack && pPack->P_SBIICfg && pPack->P_SBIICfg->P_OpList) ? pPack->P_SBIICfg->P_OpList->getCount() : 0;
			PPIDArray acc_sheets;
			SBIIOpInfoArray * p_oplist = (count) ? pPack->P_SBIICfg->P_OpList : 0;

			acc_sheets.add(acc_sheet_id);
			for(uint j = 0; j < count; j++) {
				PPID op_id = (p_oplist->at(j).OpID > 0) ? p_oplist->at(j).OpID : p_oplist->at(j).ToHostOpID;
				PPOprKind op_data;
				if(GetOpData(op_id, &op_data) > 0 && acc_sheets.lsearch(op_data.AccSheetID) <= 0) {
					acc_sheets.add(op_data.AccSheetID);
					for(n = 0; ar_obj.P_Tbl->EnumBySheet(op_data.AccSheetID, &n, &ar_rec) > 0;) {
						DbfRecord dbf_rec(p_dbf_tbl);
						dbf_rec.put(1, ar_rec.ID);
						SOemToChar(ar_rec.Name);
						dbf_rec.put(2, ar_rec.Name);
						dbf_rec.put(3, op_data.AccSheetID);
						THROW(p_dbf_tbl->appendRec(&dbf_rec));
					}
				}
			}
		}
		if(oneof3(bht_type_id, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
			p_dbf_tbl->close();
			SCopyFile(out_path, pPath, 0, FILE_SHARE_READ, 0);
		}
	}
	else
		ok = -1;
	CATCHZOK
	delete p_bht_rec;
	delete p_dbf_tbl;
	SFile::ZClose(&stream);
	return ok;
}

int SLAPI PPObjBHT::PrepareGoodsData(PPID bhtID, const char * pPath, const char * pPath2, PPID bhtTypeID, const PPIDArray * pAddendumGoodsIdList)
{
	int    ok = 1;
	PPID   loc_id = 0;
	IterCounter counter;
	long   pos_id, rec_no = 0;
	SString out_path;
	FILE * stream = 0, * stream2 = 0;
	BhtRecord * p_bht_rec = 0;
	PPIDArray goods_id_list;
	PPObjGoods goods_obj;
	PPGoodsConfig goods_cfg;
	BarcodeArray barcode_list;
	PPObjGoods::ReadConfig(&goods_cfg);
	const  size_t check_dig = BIN(goods_cfg.Flags & GCF_BCCHKDIG);
	StringSet ss;
	TempOrderTbl::Key1 tmp_k1;
	Goods2Tbl::Rec goods_rec;
	SString temp_buf;
	SString goods_name;
	SString msg_buf;
	SString barcode_buf;
	GoodsFilt filt;
	TempOrderTbl * p_tmp_tbl = 0;
	BExtInsert * p_bei = 0;
	DbfTable * p_dbf_tbl = 0; // AHTOXA
	{
		PPUserFuncProfiler ufp(PPUPRF_BHTPREPGOODS); // @v9.4.11
		double prf_measure = 0.0;
		PPLoadText(PPTXT_PREPAREBHTGOODS, msg_buf);
		// ATHOXA {
		PPSetAddedMsgString(pPath);
		if(!oneof3(bhtTypeID, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
			THROW_PP(stream = fopen(pPath, "w"), PPERR_CANTOPENFILE);
			if(pPath2)
				THROW_PP_S(stream2 = fopen(pPath2, "w"), PPERR_CANTOPENFILE, pPath2);
			THROW_MEM(p_bht_rec = new BhtRecord);
			InitGoodsBhtRec(p_bht_rec);
		}
		else {
			int    num_flds = 0;
			SString temp_path;
			DBFCreateFld fld_list[32];
			PPGetPath(PPPATH_OUT, temp_path);
			SPathStruc::ReplacePath((out_path = pPath), temp_path, 1);
			THROW(p_dbf_tbl = new DbfTable(out_path));
			fld_list[num_flds++].Init("ID",   'N', 10, 0);
			fld_list[num_flds++].Init("CODE", 'C', 16, 0);
			fld_list[num_flds++].Init("NAME", 'C', 64, 0);
			if(oneof2(bhtTypeID, PPObjBHT::btPalm, PPObjBHT::btStyloBhtII))
				fld_list[num_flds++].Init("UPP",  'N', 10, 2);
			if(oneof2(bhtTypeID, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII))
				fld_list[num_flds++].Init("PRICE", 'N', 10, 2);
			if(bhtTypeID == PPObjBHT::btStyloBhtII) {
				fld_list[num_flds++].Init("REST", 'N', 10, 2);
				fld_list[num_flds++].Init("COST", 'N', 10, 2);
			}
			THROW(p_dbf_tbl->create(num_flds, fld_list));
			THROW(p_dbf_tbl->open());
		}
		// } AHTOXA
		THROW(p_tmp_tbl = CreateTempOrderFile());
		THROW_MEM(p_bei = new BExtInsert(p_tmp_tbl));
		pos_id = 0;
		filt.ReadFromProp(Obj, bhtID, BHTPRP_GOODSFILT2, BHTPRP_GOODSFILT_);
		GoodsIterator::GetListByFilt(&filt, &goods_id_list);
		goods_id_list.add(pAddendumGoodsIdList);
		goods_id_list.sortAndUndup();
		counter.Init(goods_id_list.getCount() * 2);
		//
		// ��� ��������� � ���������� StyloBhtII, �� ������ ����� ����������� ���������� ��� ������� ���������. �������� ����� � ��������� bcode_uniq_bias.
		//
		const long bcode_uniq_bias = 26L;
		//while(giter.Next(&goods_rec) > 0) {
		for(uint j = 0; j < goods_id_list.getCount(); j++) {
			prf_measure += 1.0; // @v9.4.11
			const PPID _goods_id = goods_id_list.get(j);
			uint   i;
			BarcodeTbl::Rec * p_barcode = 0;
			THROW(goods_obj.ReadBarcodes(_goods_id, barcode_list));
			for(i = 0; barcode_list.enumItems(&i, (void**)&p_barcode);) {
				if(p_barcode->Code[0]) {
					PPID   goods_id = _goods_id;
					temp_buf = p_barcode->Code;
					if(temp_buf.Len() > 3 && temp_buf.Len() < 7) { // @v9.8.7 (temp_buf.Len() > 3 &&)
						temp_buf.PadLeft(12-temp_buf.Len(), '0');
						if(check_dig)
							AddBarcodeCheckDigit(temp_buf);
					}
					int    skip = 0;
					if(bhtTypeID == PPObjBHT::btStyloBhtII) {
						long addendum = (((long)i - 1) << bcode_uniq_bias);
						if((addendum + goods_id) < 0)
							skip = 1;
						else
							goods_id += addendum;
					}
					if(!skip) {
						ss.clear();
						ss.add(temp_buf);                     // barcode
						ss.add(temp_buf.Z().Cat(goods_id)); // goodsid
						p_tmp_tbl->clearDataBuf();
						memcpy(p_tmp_tbl->data.Name, ss.getBuf(), ss.getDataLen());
						p_tmp_tbl->data.ID = ++pos_id;
						THROW_DB(p_bei->insert(&p_tmp_tbl->data));
					}
				}
			}
			PPWaitPercent(counter.Increment(), msg_buf);
		}
		THROW_DB(p_bei->flash());
		ZDELETE(p_bei);
		{
			RECORDNUMBER nr;
			p_tmp_tbl->getNumRecs(&nr);
			counter.SetTotal(counter.GetTotal() / 2 + nr);
		}
		MEMSZERO(tmp_k1);
		while(p_tmp_tbl->search(1, &tmp_k1, spGt)) {
			//char   barcode[32];
			char   price_buf[32];
			uint   p = 0;
			ss.setBuf(p_tmp_tbl->data.Name, sizeof(p_tmp_tbl->data.Name));
			ss.get(&p, barcode_buf);
			ss.get(&p, temp_buf);
			PPID   out_goods_id = temp_buf.ToLong();
			PPID   goods_id = out_goods_id & ((1 << bcode_uniq_bias) - 1);
			if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
				double price = 0.0;
				ReceiptTbl::Rec lot_rec;
				MEMSZERO(lot_rec);
				if(goods_rec.Flags & GF_UNLIM) {
					const QuotIdent qi(QIDATE(getcurdate_()), loc_id, PPQUOTK_BASE, 0L /* @curID */);
					goods_obj.GetQuot(goods_id, qi, 0L, 0L, &price);
				}
				else
					::GetCurGoodsPrice(goods_id, loc_id, GPRET_INDEF, &price, &lot_rec);
				realfmt(price, MKSFMTD(6, 2, NMBF_NOTRAILZ), price_buf);
				if(!oneof3(bhtTypeID, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
					p_bht_rec->PutInt(0, goods_id);
					p_bht_rec->PutStr(1, barcode_buf);
					//
					// @v4.0.8 p_bht_rec->PutStr(2, price_buf);
					// int �� bht �� -32xxx �� 32767 -> ��� ���. ������ 32767 + 1 = -32xxx -> ������
					// ������� ����. � �������� 32766 ������� � ���������� (���� ������� > 32766)
					//
					if((rec_no < MAXBHTRECS - 1) || !stream2)
						PutBhtRecToFile(p_bht_rec, stream);
					else
						PutBhtRecToFile(p_bht_rec, stream2);
				}
				else {
					int    num_fld = 1;
					GoodsStockExt gse;
					DbfRecord dbf_rec(p_dbf_tbl);
					dbf_rec.put(num_fld++, out_goods_id);
					goods_name = goods_rec.Name;
					if(bhtTypeID == PPObjBHT::btStyloBhtII)
						goods_name.Trim(63);
					else
						goods_name.Trim(47);
					if(oneof2(bhtTypeID, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
						goods_name.Transf(CTRANSF_INNER_TO_OUTER);
						barcode_buf.Transf(CTRANSF_INNER_TO_OUTER);
					}
					else
						goods_name.ToLower();
					dbf_rec.put(num_fld++, barcode_buf);
					dbf_rec.put(num_fld++, goods_name);
					if(oneof2(bhtTypeID, PPObjBHT::btPalm, PPObjBHT::btStyloBhtII)) {
						if(goods_obj.GetStockExt(goods_id, &gse) > 0 && gse.Package > 0)
							dbf_rec.put(num_fld++, gse.Package);
						else
							dbf_rec.put(num_fld++, lot_rec.UnitPerPack);
					}
					if(oneof2(bhtTypeID, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII))
						dbf_rec.put(num_fld++, price);
					if(bhtTypeID == PPObjBHT::btStyloBhtII) {
						dbf_rec.put(num_fld++, lot_rec.Rest);
						dbf_rec.put(num_fld++, lot_rec.Cost);
					}
					THROW(p_dbf_tbl->appendRec(&dbf_rec));
				}
			}
			PPWaitPercent(counter.Increment(), msg_buf);
			rec_no++;
		}
		if(oneof3(bhtTypeID, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
			p_dbf_tbl->close();
			SCopyFile(out_path, pPath, 0, FILE_SHARE_READ, 0);
		}
		ufp.SetFactor(0, prf_measure); // @v9.4.11
		ufp.Commit(); // @v9.4.11
	}
	CATCHZOK
	delete p_bei;
	delete p_tmp_tbl;
	delete p_bht_rec;
	delete p_dbf_tbl;
	SFile::ZClose(&stream);
	SFile::ZClose(&stream2);
	return ok;
}

int SLAPI PPObjBHT::TransmitSuppl(BhtProtocol * pBP, int updateData)
{
	int    ok = 1, r;
	BhtRecord * p_bht_rec = 0;
	SString path;
	PPWait(1);
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_BHT_SUPPL, path) > 0);
	if(!fileExists(path) || updateData) {
		THROW(r = PrepareSupplData(path));
	}
	else
		r = 1;
	if(r > 0) {
		PPWait(0);
		if(CONFIRM(PPCFM_BHT_SENDSUPPL)) {
			PPWait(1);
			THROW_MEM(p_bht_rec = new BhtRecord);
			InitSupplBhtRec(p_bht_rec);
			THROW(pBP->SendDataFile(path, p_bht_rec));
		}
	}
	CATCHZOK
	PPWait(0);
	delete p_bht_rec;
	return ok;
}

int SLAPI PPObjBHT::TransmitSuppl(CipherProtocol * pCP, int updateData)
{
	int    ok = 1, r;
	BhtRecord * p_bht_rec = 0;
	SString path;
	PPWait(1);
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_BHT_SUPPL, path) > 0);
	if(!fileExists(path) || updateData) {
		THROW(r = PrepareSupplData(path));
	}
	else
		r = 1;
	if(r > 0) {
		PPWait(0);
		if(CONFIRM(PPCFM_BHT_SENDSUPPL)) {
			PPWait(1);
			THROW_MEM(p_bht_rec = new BhtRecord);
			InitSupplBhtRec(p_bht_rec);
			THROW(pCP->SendDataFile(path, p_bht_rec));
		}
	}
	CATCHZOK
	PPWait(0);
	delete p_bht_rec;
	return ok;
}

int SLAPI PPObjBHT::TransmitGoods(PPID bhtID, BhtProtocol * pBP, int updateData)
{
	int    ok = 1, r;
	BhtRecord * p_bht_rec = 0;
	//char   path[MAXPATH], path2[MAXPATH];
	SString path;
	SString path2;
	PPWait(1);
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_BHT_GOODS, path) > 0);
	{
		//
		// ������������ ����� ��������������� ����� ������� �� ������,
		// ���� ���������� ������� ��������� 32K
		//
		SPathStruc ps(path);
		ps.Nam.Cat("2");
		ps.Merge(path2);
	}
	if(!fileExists(path) || !fileExists(path2) || updateData) {
		THROW(r = PrepareGoodsData(bhtID, path, path2, 0, 0));
	}
	else
		r = 1;
	if(r > 0) {
		PPWait(0);
		if(CONFIRM(PPCFM_BHT_SENDGOODS)) {
			PPWait(1);
			THROW_MEM(p_bht_rec = new BhtRecord);
			InitGoodsBhtRec(p_bht_rec);
			THROW(pBP->SendDataFile(path, p_bht_rec));

			ZDELETE(p_bht_rec);
			THROW_MEM(p_bht_rec = new BhtRecord);
			InitGoodsBhtRec(p_bht_rec);
			THROW(pBP->SendDataFile(path2, p_bht_rec));
		}
		else
			ok = -1;
	}
	CATCHZOK
	PPWait(0);
	delete p_bht_rec;
	return ok;
}

int SLAPI PPObjBHT::TransmitGoods(PPID bhtID, CipherProtocol * pCP, int updateData)
{
	int    ok = 1, r;
	BhtRecord * p_bht_rec = 0;
	SString path;
	PPWait(1);
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_BHT_GOODS, path) > 0);
	if(!fileExists(path) || updateData) {
		THROW(r = PrepareGoodsData(bhtID, path, 0, 0, 0));
	}
	else
		r = 1;
	if(r > 0) {
		PPWait(0);
		if(CONFIRM(PPCFM_BHT_SENDGOODS)) {
			PPWait(1);
			THROW_MEM(p_bht_rec = new BhtRecord);
			InitGoodsBhtRec(p_bht_rec);
			THROW(pCP->SendDataFile(path, p_bht_rec));
		}
		else
			ok = -1;
	}
	CATCHZOK
	PPWait(0);
	delete p_bht_rec;
	return ok;
}

int SLAPI PPObjBHT::AcceptBill(PPObjBHT::BillRec * pRec, PPBasketPacket * pGBPack, PPID * pAltGrpID)
{
	int    ok = -1;
	if(pRec) {
		SString temp_buf;
		PPObjArticle ar_obj;
		ArticleTbl::Rec ar_rec;
		temp_buf.Cat(pRec->ID).Space();
		if(ar_obj.Fetch(pRec->SupplID, &ar_rec) > 0)
			temp_buf.Cat(ar_rec.Name).Space();
		temp_buf.Cat(pRec->Dt, DATF_DMY);
		if(pGBPack) {
			int    r = 0;
			int    gb_exists = 0;
			PPID   gb_id = 0;
			PPBasketPacket gb_packet;
			PPObjGoodsBasket gb_obj;
			THROW((r = gb_obj.SearchByName(&gb_id, temp_buf, &gb_packet)));
			if(r != 1) {
				MEMSZERO(gb_packet.Head);
				gb_packet.Head.SupplID = pRec->SupplID;
				STRNSCPY(gb_packet.Head.Name, temp_buf);
				gb_packet.GoodsID = 0;
				gb_exists = 0;
			}
			else if(PPObjGoodsBasket::IsLocked(gb_id))
				gb_exists = 1;
			if(!gb_exists) {
				ASSIGN_PTR(pGBPack, gb_packet);
				ok = 1;
			}
		}
		else if(pAltGrpID) {
			PPID   alt_grp_id = 0;
			PPGoodsPacket gp;
			THROW(GGObj.InitPacket(&gp, gpkndAltGroup, 0, 0, 0));
			STRNSCPY(gp.Rec.Name, temp_buf);
			if(GGObj.SearchByName(gp.Rec.Name, &alt_grp_id, 0) <= 0) {
				THROW(GGObj.PutPacket(&alt_grp_id, &gp, 0));
			}
			ASSIGN_PTR(pAltGrpID, alt_grp_id);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBHT::AcceptBillLine(PPID billID, PPObjBHT::BillLineRec * pRec, PPBasketPacket * pGBPack, PPID * pAltGrpID)
{
	int ok = -1;
	if(pRec && billID == pRec->BillID) {
		if(pGBPack) {
			if(pRec->Quantity > 0 && GObj.Search(pRec->GoodsID) > 0) {
				uint pos = 0;
				ILTI   item;
				item.GoodsID  = pRec->GoodsID;
				item.Quantity = pRec->Quantity;
				item.Price    = pRec->Price;
				item.Expiry   = pRec->Expiry;
				if(!checkdate(item.Expiry, 1))
					item.Expiry = ZERODATE;
				if(pGBPack->SearchGoodsID(item.GoodsID, &pos) > 0) {
					item.Quantity += pGBPack->Lots.at(pos).Quantity;
					pGBPack->DelItem(pos);
				}
				THROW(ok = pGBPack->AddItem(&item, 0));
			}
		}
		else if(pAltGrpID && *pAltGrpID) {
			THROW(ok = GGObj.AssignGoodsToAltGrp(pRec->GoodsID, *pAltGrpID, 0, 0));
		}
	}
	CATCHZOK
	return ok;
}

static int FASTCALL __RemoveUndupNameSuffix(SString & rBuf)
{
	int    ok = -1;
	size_t pos = rBuf.Len();
	uint   nd = 0;
	if(pos) do {
		const char c = rBuf.C(--pos);
		if(c == '#') {
			if(pos > 5 && nd > 0 && nd < 3) {
				rBuf.Trim(pos-1).Strip();
				ok = 1;
			}
		}
		else if(c >= '0' && c <= '9') {
			nd++;
		}
		else
			break;
	} while(ok < 0 && pos);
	return ok;
}

int SLAPI PPObjBHT::AcceptInvent(PPID opID, PPObjBHT::InventRec * pRec, BillTbl::Rec * pInvRec, PPLogger * pLogger)
{
	int    ok = -1;
	THROW_PP(opID, PPERR_INVOPNOTDEF);
	if(pRec) {
		SString bill_code = pRec->Code;
		LDATE  inv_dt = pRec->Dt;
		PPID   bid = pRec->ID;
		PPID   inv_id = 0;
		int    dup_found = 0;
		BillTbl::Rec inv_rec;
		PPBillPacket pack;
		SETIFZ(inv_dt, LConfig.OperDate);
		BillTbl::Rec bill_rec;
		if(!pRec->Uuid.IsZero()) {
			if(P_BObj->SearchByGuid(pRec->Uuid, &bill_rec) > 0) {
				if(pLogger) {
					SString fmt_buf, msg_buf, guid_buf;
					// ������ �������� ������������� �������������� �� GUID=%s. ����� �������� ������ �� �����.
					pRec->Uuid.ToStr(S_GUID::fmtIDL, guid_buf);
					pLogger->Log(msg_buf.Printf(PPLoadTextS(PPTXT_IMP_INFDUPGUIDFOUND, fmt_buf), guid_buf.cptr()));
				}
				dup_found = 1;
			}
		}
		else {
			// @v8.0.9 {
			long   uc = 0;
			int    new_code = 0;
			do {
				new_code = 0;
				for(DateIter di(inv_dt, inv_dt); !new_code && P_BObj->P_Tbl->EnumByOpr(opID, &di, &bill_rec) > 0;) {
					if(bill_code.CmpNC(bill_rec.Code) == 0) {
						__RemoveUndupNameSuffix(bill_code);
						bill_code.Strip().Space().Cat('#').Cat(++uc);
						new_code = 1;
					}
				}
			} while(new_code);
			if(uc) {
				if(pLogger) {
					// ������ �������� ������������� �������������� �� ���� � ����. ����� �������� ����� ����� ��� '%s'
					SString fmt_buf, msg_buf;
					pLogger->Log(msg_buf.Printf(PPLoadTextS(PPTXT_IMP_INVDUPFOUND, fmt_buf), bill_code.cptr()));
				}
			}
			// } @v8.0.9
		}
		if(!dup_found) {
			if(!inv_id) {
				THROW(pack.CreateBlank(opID, 0, 0, 0));
				bill_code.CopyTo(pack.Rec.Code, sizeof(pack.Rec.Code));
				pack.Rec.Dt = inv_dt;
				THROW(P_BObj->TurnInventory(&pack, 0));
				inv_id = pack.Rec.ID;
				inv_rec = pack.Rec;
			}
			if(inv_id) {
				ASSIGN_PTR(pInvRec, inv_rec);
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

// static
int SLAPI PPObjBHT::AcceptBillsToGBasketPalm(const char * pHName, const char * pLName, PPLogger * pLog)
{
	int    ok = -1;
	DbfTable bill_tbl(pHName);
	DbfTable line_tbl(pLName);
	PPObjGoods       g_obj;
	PPObjGoodsBasket gb_obj;
	if(bill_tbl.getNumRecs() > 0 && bill_tbl.top()) {
		SString temp_buf, suppl_name;
		int    fldn_id = 0, fldn_dt = 0, fldn_suppl = 0;
		int    fldn_bid = 0, fldn_gid = 0, fldn_qtty = 0, fldn_price = 0, fldn_expiry = 0;
		bill_tbl.getFieldNumber("ID",      &fldn_id);
		bill_tbl.getFieldNumber("DATE",    &fldn_dt);
		bill_tbl.getFieldNumber("SUPPLID", &fldn_suppl);

		line_tbl.getFieldNumber("BILLID",   &fldn_bid);
		line_tbl.getFieldNumber("GOODSID",  &fldn_gid);
		line_tbl.getFieldNumber("QUANTITY", &fldn_qtty);
		line_tbl.getFieldNumber("PRICE",    &fldn_price);
		line_tbl.getFieldNumber("EXPIRY",   &fldn_expiry);
		do {
			if(!bill_tbl.isDeletedRec()) {
				char   str_dt[16];
				int    gb_exists = 0, r = 0;
				long   bills_num = 0;
				PPID   bid = 0;
				PPID   suppl_id = 0, gb_id = 0;
				LDATE  dt = ZERODATE;
				PPBasketPacket gb_packet;
				DbfRecord b_rec(&bill_tbl);

				r = 0;
				bills_num = 0;
				suppl_id = gb_id = 0;

				bill_tbl.getRec(&b_rec);
				b_rec.get(fldn_id, bid);
				b_rec.get(fldn_dt, dt);
				b_rec.get(fldn_suppl, suppl_id);
				GetArticleName(suppl_id, suppl_name);
				datefmt(&dt, DATF_DMY, str_dt);
				temp_buf.Z().Cat(bid).Space().Cat(str_dt).Space().Cat(suppl_name);
				THROW((r = gb_obj.SearchByName(&gb_id, temp_buf, &gb_packet)));
				if(r != 1) {
					MEMSZERO(gb_packet.Head);
					gb_packet.Head.SupplID = suppl_id;
					temp_buf.CopyTo(gb_packet.Head.Name, sizeof(gb_packet.Head.Name));
					gb_packet.GoodsID = 0;
					gb_exists = 0;
				}
				else if(PPObjGoodsBasket::IsLocked(gb_id))
					gb_exists = 1;
				if(!gb_exists && line_tbl.top()) {
					do {
						if(!line_tbl.isDeletedRec()) {
							PPID   lbid = 0, gid = 0;
							double qtty = 0.0, price = 0.0;
							LDATE  expiry = ZERODATE;
							ILTI   item;
							DbfRecord bl_rec(&line_tbl);

							line_tbl.getRec(&bl_rec);
							bl_rec.get(fldn_bid, lbid);
							bl_rec.get(fldn_gid, gid);
							bl_rec.get(fldn_qtty, qtty);
							bl_rec.get(fldn_price, price);
							bl_rec.get(fldn_expiry, dt);
							if(lbid == bid && /*qtty > 0 && */ g_obj.Search(gid) > 0) { // @v5.1.8 AHTOXA
								uint pos = 0;
								item.GoodsID  = gid;
								item.Quantity = qtty;
								item.Price    = price;
								item.Expiry = expiry;
								if(!checkdate(item.Expiry, 1))
									item.Expiry = ZERODATE;
								if(gb_packet.SearchGoodsID(item.GoodsID, &pos) > 0) {
									item.Quantity += gb_packet.Lots.at(pos).Quantity;
									gb_packet.DelItem(pos);
								}
								THROW(gb_packet.AddItem(&item, 0));
								bills_num++;
							}
							ok = 1;
						}
					} while(line_tbl.next());
					if(bills_num > 0)
						THROW(gb_obj.PutPacket(&gb_id, &gb_packet, 1));
				}
			}
		} while(bill_tbl.next());
	}
	CATCHZOK
	return ok;
}

// static
int SLAPI PPObjBHT::AcceptInventPalm(const char * pHName, const char * pLName, PPID opID, PPLogger * pLog)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	PPBillPacket pack;
	DbfTable bill_tbl(pHName);
	DbfTable line_tbl(pLName);
	InventoryCore & r_inv_tbl = p_bobj->GetInvT();
	THROW_PP(opID, PPERR_INVOPNOTDEF);
	{
		PPTransaction tra(1);
		THROW(tra);
		if(bill_tbl.getNumRecs() && bill_tbl.top()) {
			int fldn_id = 0, fldn_dt  = 0;
			int fldn_bid = 0, fldn_gid = 0, fldn_qtty = 0;
			bill_tbl.getFieldNumber("ID",   &fldn_id);
			bill_tbl.getFieldNumber("DATE", &fldn_dt);

			line_tbl.getFieldNumber("INVID",   &fldn_bid);
			line_tbl.getFieldNumber("GOODSID",  &fldn_gid);
			line_tbl.getFieldNumber("QUANTITY",  &fldn_qtty);
			do {
				if(!bill_tbl.isDeletedRec()) {
					PPObjBill::InvBlock inv_blk;
					LDATE  inv_dt = ZERODATE;
					PPID   inv_id = 0, bid = 0;
					DbfRecord b_rec(&bill_tbl);
					bill_tbl.getRec(&b_rec);
					b_rec.get(fldn_id, bid);
					b_rec.get(fldn_dt, inv_dt);
					SETIFZ(inv_dt, LConfig.OperDate);
					if(line_tbl.top()) {
						do {
							if(!line_tbl.isDeletedRec()) {
								PPID   lbid;
								PPID   goods_id = 0;
								double qtty = 0.0;
								DbfRecord bl_rec(&line_tbl);
								line_tbl.getRec(&bl_rec);
								bl_rec.get(fldn_bid, lbid);
								bl_rec.get(fldn_gid,  goods_id);
								bl_rec.get(fldn_qtty, qtty);
								if(bid == lbid && goods_id && qtty > 0.0) {
									if(!inv_id) {
										char   bill_code[32];
										sprintf(bill_code, "%0.5ld", bid);
										strcat(bill_code, "B");
										DateIter di(inv_dt, inv_dt);
										BillTbl::Rec bill_rec;
										while(!inv_id && p_bobj->P_Tbl->EnumByOpr(opID, &di, &bill_rec) > 0)
											if(stricmp(bill_rec.Code, bill_code) == 0)
												inv_id  = bill_rec.ID;
										if(!inv_id) {
											THROW(pack.CreateBlank(opID, 0, 0, 0));
											STRNSCPY(pack.Rec.Code, bill_code);
											pack.Rec.Dt = inv_dt;
											THROW(p_bobj->TurnInventory(&pack, 0));
											inv_id  = pack.Rec.ID;
										}
										THROW(p_bobj->InitInventoryBlock(inv_id, inv_blk));
									}
									{
										PPObjBill::InvItem inv_item;
										inv_item.Init(goods_id, 0);
										inv_item.Qtty = qtty;
										if(!p_bobj->AcceptInventoryItem(inv_blk, &inv_item, 0)) {
											if(pLog)
												pLog->LogLastError();
											else {
												CALLEXCEPT();
											}
										}
										ok = 1;
									}
								}
							}
						} while(line_tbl.next());
					}
					if(inv_id) {
						InventoryTotal total;
						InventoryFilt filt;
						THROW(p_bobj->ExtractPacket(inv_id, &pack) > 0);
						filt.SetSingleBillID(pack.Rec.ID);
						THROW(r_inv_tbl.CalcTotal(&filt, &total));
						pack.Rec.Amount = BR2(total.Amount);
						pack.Amounts.Put(PPAMT_MAIN, pack.Rec.CurID, total.Amount, 0, 1);
						THROW(p_bobj->TurnInventory(&pack, 0));
						DS.LogAction(PPACN_INVENTBYTERM, PPOBJ_BILL, inv_id, 0, 0);
					}
				}
			} while(bill_tbl.next());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBHT::AcceptTechSessPalm(const char * pLName, PPLogger * pLog)
{
	int    ok = -1;
	int    is_first_rec = 1;
	BhtTSessRec tses_rec;
	DbfTable line_tbl(pLName);
	PPObjTSession ts_obj;
	PPObjGoods goods_obj;
	{
		PPTransaction tra(1);
		THROW(tra);
		if(line_tbl.getNumRecs() && line_tbl.top()) {
			SString tim_buf;
			int    fldn_bill, fldn_prc = 0, fldn_ar = 0, fldn_serial = 0, fldn_dt = 0, fldn_tm = 0, fldn_qtty = 0;
			line_tbl.getFieldNumber("BILLID",  &fldn_bill);
			line_tbl.getFieldNumber("PRCCODE", &fldn_prc);
			line_tbl.getFieldNumber("ARCODE",  &fldn_ar);
			line_tbl.getFieldNumber("SERIAL",  &fldn_serial);
			line_tbl.getFieldNumber("DT",      &fldn_dt);
			line_tbl.getFieldNumber("TM",      &fldn_tm);
			line_tbl.getFieldNumber("QTTY",    &fldn_qtty);
			do {
				DbfRecord tsl_rec(&line_tbl);
				line_tbl.getRec(&tsl_rec);
				MEMSZERO(tses_rec);
				tsl_rec.get(fldn_bill,   tses_rec.BillCode);
				tsl_rec.get(fldn_prc,    tses_rec.PrcCode);
				tsl_rec.get(fldn_ar,     tses_rec.ArCode);
				tsl_rec.get(fldn_serial, tses_rec.Barcode);
				tsl_rec.get(fldn_qtty,   tses_rec.Qtty);
				tsl_rec.get(fldn_dt,     tses_rec.Dtm.d);
				tsl_rec.get(fldn_tm,     tim_buf);
				strtotime(tim_buf, TIMF_HMS, &tses_rec.Dtm.t);
				strip(tses_rec.BillCode);
				strip(tses_rec.PrcCode);
				strip(tses_rec.ArCode);
				strip(tses_rec.Barcode);
				int    sig;
				if(strcmp(tses_rec.PrcCode, "PRCCLRL") == 0)
					sig = 3;
				else if(is_first_rec)
					sig = 1;
				else
					sig = 0;
				ts_obj.ProcessBhtRec(sig, &tses_rec, pLog, 0);
				is_first_rec = 0;
			} while(line_tbl.next());
			MEMSZERO(tses_rec);
			ts_obj.ProcessBhtRec(2, &tses_rec, pLog, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBHT::AcceptBillsPalm(const char * pHName, const char * pLName, PPLogger * pLog)
{
	int    ok = -1;
	PPObjGoodsGroup gg_obj;
	DbfTable bill_tbl(pHName);
	DbfTable line_tbl(pLName);
	{
		PPTransaction tra(1);
		THROW(tra);
		if(bill_tbl.getNumRecs() && bill_tbl.top() && line_tbl.getNumRecs()) {
			SString temp_buf, suppl_name;
			int fldn_id = 0, fldn_dt  = 0, fldn_suppl = 0;
			int fldn_bid = 0, fldn_gid = 0;
			bill_tbl.getFieldNumber("ID",   &fldn_id);
			bill_tbl.getFieldNumber("DATE", &fldn_dt);
			bill_tbl.getFieldNumber("SUPPLID", &fldn_suppl);

			line_tbl.getFieldNumber("BILLID",   &fldn_bid);
			line_tbl.getFieldNumber("GOODSID",  &fldn_gid);
			do {
				if(!bill_tbl.isDeletedRec()) {
					PPID   alt_grp_id = 0, bid = 0;
					PPID   suppl_id = 0;
					DbfRecord b_rec(&bill_tbl);
					LDATE  dt = ZERODATE;

					bill_tbl.getRec(&b_rec);
					b_rec.get(fldn_id, bid);
					b_rec.get(fldn_dt, dt);
					b_rec.get(fldn_suppl, suppl_id);
					GetArticleName(suppl_id, suppl_name);
					if(line_tbl.top()) {
						do {
							if(!line_tbl.isDeletedRec()) {
								PPID   lbid = 0, gid = 0;
								DbfRecord bl_rec(&line_tbl);
								line_tbl.getRec(&bl_rec);
								bl_rec.get(fldn_bid, bid);
								bl_rec.get(fldn_gid, gid);
								if(bid == lbid) {
									if(alt_grp_id == 0) {
										PPID   temp_id = 0;
										char   str_dt[16];
										PPGoodsPacket gp;
										THROW(gg_obj.InitPacket(&gp, gpkndAltGroup, 0, 0, 0));
										datefmt(&dt, DATF_DMY, str_dt);
										temp_buf.Z().Cat(bid).Space().Cat(str_dt).Space().Cat(suppl_name);
										STRNSCPY(gp.Rec.Name, temp_buf);
										if(gg_obj.SearchByName(gp.Rec.Name, &temp_id, 0) <= 0) {
											PPWaitMsg(gp.Rec.Name);
											THROW(gg_obj.PutPacket(&alt_grp_id, &gp, 0));
										}
										else
											break;
									}
									// @todo Process errors at next call
									gg_obj.AssignGoodsToAltGrp(gid, alt_grp_id, 0, 0);
									ok = 1;
								}
							}
						} while(line_tbl.next());
					}
				}
			} while(bill_tbl.next());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

IMPL_CMPFUNC(Sdr_SBIIBillRow, i1, i2)
{
	const Sdr_SBIIBillRow * p_i1 = (const Sdr_SBIIBillRow*)i1;
	const Sdr_SBIIBillRow * p_i2 = (const Sdr_SBIIBillRow*)i2;
	if(p_i1->Number < p_i2->Number)
		return -1;
	else if(p_i1->Number > p_i2->Number)
		return 1;
	else
		return 0;
}

static int SLAPI GetBillRows(const char * pLName, TSVector <Sdr_SBIIBillRow> * pList) // @v9.8.4 TSArray-->TSVector
{
	int    ok = 1;
	PPImpExpParam ie_param_brow;
	THROW_INVARG(pList);
	THROW(InitImpExpDbfParam(PPREC_SBIIBILLROW, &ie_param_brow, pLName, 0));
	{
		PPObjBill * p_bobj = BillObj;
		long   rows_count = 0;
		PPID   common_goods_id = 0;
		SString serial;
		PPIDArray  lot_list;
		PPImpExp ie_brow(&ie_param_brow, 0);
		ResolveGoodsItemList unknown_goods_list;
		ReceiptCore  & r_rcpt = p_bobj->trfr->Rcpt;
		Goods2Tbl::Rec goods_rec;
		PPObjGoods     gobj;
		// @v9.7.0 if(gobj.SearchByBarcode(CConfig.PrepayInvoiceGoodsCode, 0, &goods_rec) > 0)
		if(gobj.Search(CConfig.PrepayInvoiceGoodsID, &goods_rec) > 0) // @v9.7.0 
			common_goods_id = goods_rec.ID;
		THROW(ie_brow.OpenFileForReading(0));
		pList->freeAll();
		ie_brow.GetNumRecs(&rows_count);
		for(long j = 0; j < rows_count; j++) {
			Sdr_SBIIBillRow sdr_brow;
			MEMSZERO(sdr_brow);
			THROW(ie_brow.ReadRecord(&sdr_brow, sizeof(sdr_brow)));
			//
			// ����� ������ �� �������� ������, �����, ���� �� ������, �� ���������
			//
			(serial = sdr_brow.Serial).Strip();
			if(gobj.Search(sdr_brow.GoodsID, 0) <= 0)
				sdr_brow.GoodsID = 0;
			if(!sdr_brow.GoodsID) {
				lot_list.clear();
				if(p_bobj->SearchLotsBySerial(serial, &lot_list) > 0 && lot_list.getCount()) {
	 				ReceiptTbl::Rec lot_rec;
	 				THROW(r_rcpt.Search(lot_list.at(0), &lot_rec) > 0); // @todo softerror
	 				sdr_brow.GoodsID = lot_rec.GoodsID;
				}
	 		}
	 		if(!sdr_brow.GoodsID) {
	 			GoodsCodeSrchBlock srch_blk;
	 			srch_blk.Flags = GoodsCodeSrchBlock::fGoodsId;
	 			serial.CopyTo(srch_blk.Code, sizeof(srch_blk.Code));
	 			if(gobj.SearchByCodeExt(&srch_blk) > 0)
		 			sdr_brow.GoodsID = srch_blk.GoodsID;
		 	}
		 	pList->insert(&sdr_brow);
		 	if(!sdr_brow.GoodsID && unknown_goods_list.lsearch(sdr_brow.Serial, 0, PTR_CMPFUNC(Pchar), offsetof(ResolveGoodsItem, Barcode)) <= 0) {
	 			ResolveGoodsItem goods_item;
	 			STRNSCPY(goods_item.Barcode, sdr_brow.Serial);
	 			goods_item.Quantity = sdr_brow.Qtty;
	 			unknown_goods_list.insert(&goods_item);
	 		}
	 	}
		if(unknown_goods_list.getCount()) {
			PPWait(0);
			if(ResolveGoodsDlg(&unknown_goods_list, RESOLVEGF_SHOWBARCODE|RESOLVEGF_SHOWQTTY|RESOLVEGF_MAXLIKEGOODS|RESOLVEGF_SHOWEXTDLG) > 0) {
	 			for(uint k = 0; k < unknown_goods_list.getCount(); k++) {
		 			const ResolveGoodsItem & r_goods_item = unknown_goods_list.at(k);
 					for(uint pos = 0; pList->lsearch(r_goods_item.Barcode, &pos, PTR_CMPFUNC(Pchar), offsetof(Sdr_SBIIBillRow, Serial)); pos++)
						pList->at(pos).GoodsID = r_goods_item.ResolvedGoodsID ? r_goods_item.ResolvedGoodsID : common_goods_id;
		 		}
			}
			PPWait(1);
		}
	}
	CATCHZOK
	return ok;
}

// static
int SLAPI PPObjBHT::AcceptBillsSBII(const PPBhtTerminalPacket * pPack, PPID destIntrLocID, const char * pHName, const char * pLName, PPLogger * pLog)
{
	int    ok = -1;
	const  PPConfig & r_cfg = LConfig;
	const  PPID loc_id = r_cfg.Location;
	long   count = 0;
	PPObjBill * p_bobj = BillObj;
	SString bill_code, buf, goods_name, add_info, temp_buf;
	S_GUID  uuid;
	TSVector <Sdr_SBIIBillRow> bill_rows_list; // @v9.8.4 TSArray-->TSVector
	PPObjGoods goods_obj;
	PPObjOprKind op_obj;
	PPImpExpParam ie_param_bill;
	PPImpExp * p_ie_bill = 0;
	RetailPriceExtractor rpe(loc_id, 0, 0, ZERODATETIME, RTLPF_GETCURPRICE); // @unused
	Transfer * p_trfr = p_bobj->trfr;
	ReceiptCore & r_rcpt = p_trfr->Rcpt;
	InventoryCore & r_inv_tbl = p_bobj->GetInvT();
	PPObjBHT bht_obj;

	THROW(InitImpExpDbfParam(PPREC_SBIIBILL, &ie_param_bill, pHName, 0));
	THROW_MEM(p_ie_bill = new PPImpExp(&ie_param_bill, 0));
	THROW_INVARG(pPack);
	THROW_PP(pPack->Rec.BhtTypeID == PPObjBHT::btStyloBhtII && pPack->P_SBIICfg, PPERR_SBII_CFGNOTVALID);
	//
	// ��������� ������ ����� ���������� � ��������� ����������� ������
	//
	THROW(GetBillRows(pLName, &bill_rows_list));
	{
		PPTransaction tra(0); // @v8.6.9 (1-->0) - no scope transaction. ������ ���������� �������� - � ����������� ����������.
		THROW(tra);
		if(pPack->P_SBIICfg->Flags & StyloBhtIIConfig::fInputBillRowNumber)
			bill_rows_list.sort(PTR_CMPFUNC(Sdr_SBIIBillRow));
		THROW(p_ie_bill->OpenFileForReading(0));
		p_ie_bill->GetNumRecs(&count);
		for(long i = 0; i < count; i++) {
			int    accept_doc = 0;
			PPID   alt_grp_id = 0, op_id = 0, sign = 1;
			PPID   draft_wroff_id = 0; // �� �����-���������, ������� ����������� ����������� ����������.
			LTIME  tm = ZEROTIME;
			PPObjBHT::InventRec bht_inv_rec;
			BillTbl::Rec bill_rec;
			PPBillPacket pack, link_pack;
			Sdr_SBIIBill sdr_bill;

			MEMSZERO(sdr_bill);
			MEMSZERO(bill_rec);
			THROW(p_ie_bill->ReadRecord(&sdr_bill, sizeof(sdr_bill)));
			uuid.FromStr(sdr_bill.Guid);
			(bill_code = sdr_bill.Code).Strip();
			bill_code.Cat(pPack->P_SBIICfg->DeviceName);
			SETIFZ(sdr_bill.Date, r_cfg.OperDate);
			if(sdr_bill.OpID == StyloBhtIIConfig::oprkInventory) {
				op_id = pPack->Rec.InventOpID;
				MEMSZERO(bht_inv_rec);
				bht_inv_rec.ID = sdr_bill.ID;
				bht_inv_rec.Dt = sdr_bill.Date;
				bill_code.CopyTo(bht_inv_rec.Code, sizeof(bht_inv_rec.Code));
				bht_inv_rec.Uuid = uuid;
				int    r = bht_obj.AcceptInvent(op_id, &bht_inv_rec, &bill_rec, pLog);
				THROW(r);
				if(r > 0)
					accept_doc = 1;
			}
			else {
				int    r = p_bobj->SearchByGuid(uuid, &bill_rec);
				THROW(r);
				accept_doc = (r > 0) ? 0 : 1;
				if(accept_doc) {
					BillTbl::Rec sample_bill_rec;
					MEMSZERO(sample_bill_rec);
					if(sdr_bill.SampleID) {
						if(p_bobj->Search(sdr_bill.SampleID, &sample_bill_rec) > 0)
							op_id = pPack->P_SBIICfg->GetOpID(sample_bill_rec.OpID);
					}
					else
						op_id = pPack->P_SBIICfg->GetOpID(-sdr_bill.OpID);
					if(op_id) {
						PPTransferItem ti;
						PPID   article_id = 0;
						THROW(pack.CreateBlank(op_id, /*sdr_bill.SampleID*/0, loc_id, -1)); // @v8.6.9 use_ta 0-->-1
						bill_code.CopyTo(pack.Rec.Code, sizeof(pack.Rec.Code));
						pack.Rec.Dt     = sdr_bill.Date;
						//pack.Rec.Object = sdr_bill.Article;
						article_id = sdr_bill.Article;
						bill_rec = pack.Rec;
						sign = (ti.GetSign(op_id) == TISIGN_MINUS) ? -1 : 1;
						if(sdr_bill.SampleID) {
							if(IsDraftOp(sample_bill_rec.OpID)) {
								PPDraftOpEx doe;
								op_obj.GetDraftExData(sample_bill_rec.OpID, &doe);
								if(doe.WrOffOpID == op_id) {
									/*
									if(sample_bill_rec.Flags & BILLF_WRITEDOFF) {
										if(pLog) {
											// �������� � ��� �������� '@zstr' ������ ������� �����-�������� ������� '@bill', �� ������� ��� ������
											PPLoadText(PPTXT_BHT_DRAFTWROFF, add_info);
											PPObjBill::MakeCodeString(&pack.Rec, 0, temp_buf);
											PPFormat(add_info, &buf, temp_buf.cptr(), sample_bill_rec.ID);
											pLog->Log(buf);
										}
									}
									else */ {
										//SETIFZ(pack.Rec.Object, doe.WrOffObjID);
										SETIFZ(article_id, doe.WrOffObjID);
										pack.Rec.LinkBillID = sample_bill_rec.ID;
										draft_wroff_id = sample_bill_rec.ID;
										if(pack.Rec.LinkBillID)
											p_bobj->ExtractPacket(pack.Rec.LinkBillID, &link_pack);
									}
								}
							}
						}
						if(!article_id && IsIntrExpndOp(op_id)) {
							if(destIntrLocID) {
								if(destIntrLocID != pack.Rec.LocID) {
									article_id = PPObjLocation::WarehouseToObj(destIntrLocID);
								}
								else {
									accept_doc = 0;
									PPSetError(PPERR_PRIMEQFOREIN);
								}
							}
							else {
								accept_doc = 0;
								PPSetError(PPERR_INTRDESTNEEDED);
							}
						}
						if(article_id) {
							PPBillPacket::SetupObjectBlock sob;
							if(!pack.SetupObject(article_id, sob))
								accept_doc = 0;
						}
						if(!accept_doc) {
							CALLPTRMEMB(pLog, LogLastError());
						}
					}
					else if(pLog) {
						SString s_dt;
						s_dt.Cat(sdr_bill.Date);
						if(sdr_bill.OpID == StyloBhtIIConfig::oprkExpend)
							PPLoadString("expend", temp_buf);
						else if(sdr_bill.OpID == StyloBhtIIConfig::oprkReceipt)
							PPLoadString("incoming", temp_buf);
						else
							PPLoadString("bailment", temp_buf);
						PPGetMessage(mfError, PPERR_INVBHTTOHOSTOP, buf, DS.CheckExtFlag(ECF_SYSSERVICE), buf.Z());
						add_info.Printf(buf, temp_buf.cptr(), sdr_bill.SampleID, s_dt.cptr(), bill_code.cptr());
						buf.ShiftLeft();
						pLog->Log(buf);
						accept_doc = 0;
					}
				}
				else if(pLog) {
					PPLoadString("date", buf);
					(add_info = buf).CatChar('[').Cat(bill_rec.Dt).CatChar(']').CatDiv(',', 2);
					// @v9.3.10 PPGetWord(PPWORD_BILLCODE, 0, buf);
					PPLoadString("billno", buf); // @v9.3.10
					add_info.Cat(buf).CatChar('[').Cat(bill_code).CatChar(']');
					PPGetMessage(mfError, PPERR_DOC_ALREADY_EXISTS, add_info, DS.CheckExtFlag(ECF_SYSSERVICE), buf.Z());
					buf.ShiftLeft();
					pLog->Log(buf);
				}
			}
			if(accept_doc) {
				SString serial;
				PPIDArray lot_list;
				PPObjBill::InvBlock inv_blk;
				if(sdr_bill.OpID == StyloBhtIIConfig::oprkInventory) {
					THROW(p_bobj->InitInventoryBlock(bill_rec.ID, inv_blk));
				}
				for(long j = 0; accept_doc && j < (long)bill_rows_list.getCount(); j++) {
					Sdr_SBIIBillRow sdr_brow;
					sdr_brow = bill_rows_list.at(j);
					if(sdr_bill.ID == sdr_brow.BillID) {
						int is_serial = 0;
						(serial = sdr_brow.Serial).Strip();
						lot_list.clear();
						if(p_bobj->SearchLotsBySerial(serial, &lot_list) > 0 && lot_list.getCount()) {
							ReceiptTbl::Rec lot_rec;
							// @v9.6.4 THROW(r_rcpt.Search(lot_list.at(0), &lot_rec) > 0); // @todo softerror
							// @v9.6.4 {
							for(uint lidx = 0; !is_serial && lidx < lot_list.getCount(); lidx++) {
								if(r_rcpt.Search(lot_list.at(lidx), &lot_rec) > 0) {
									SETIFZ(sdr_brow.GoodsID, lot_rec.GoodsID);
									is_serial = 1;
								}
							}
							// } @v9.6.4
						}
						if(sdr_brow.GoodsID) {
							if(sdr_bill.OpID == StyloBhtIIConfig::oprkInventory) {
								PPObjBill::InvItem inv_item;
								inv_item.Init(sdr_brow.GoodsID, is_serial ? serial.cptr() : 0);
								inv_item.Qtty = sdr_brow.Qtty;
								inv_item.Cost = sdr_brow.Cost;
								inv_item.Price = sdr_brow.Cost;
								// ��������� ���� ����������
								{
									GoodsRestParam gr_param;
									if(p_bobj->GetInventoryStockRest(inv_blk, &inv_item, &gr_param) > 0) {
										inv_item.Price = inv_item.StockPrice;
									}
									// @v8.5.8 {
									if(inv_item.Price <= 0.0 && sdr_brow.Cost > 0.0)
										inv_item.Price = sdr_brow.Cost;
									// } @v8.5.8
								}
								if(!p_bobj->AcceptInventoryItem(inv_blk, &inv_item, -1)) { // @v8.6.9 use_ta 0-->-1
									if(pLog)
										pLog->LogLastError();
									else {
										CALLEXCEPT();
									}
								}
							}
							else if(sdr_brow.Qtty > 0.0) {
								ILTI ilti;
								if(pPack->P_SBIICfg->IsCostAsPrice((sdr_bill.SampleID) ? op_id : -sdr_bill.OpID))
									ilti.Price = sdr_brow.Cost;
								else
									ilti.Cost = sdr_brow.Cost;
								if(ilti.Cost == 0.0) {
									ReceiptTbl::Rec lot_rec;
									MEMSZERO(lot_rec);
									::GetCurGoodsPrice(sdr_brow.GoodsID, loc_id, GPRET_INDEF, 0, &lot_rec);
									ilti.Cost = lot_rec.Cost;
								}
								if(ilti.Cost == 0 && pack.Rec.LinkBillID) {
									uint ti_pos = 0;
									if(link_pack.SearchGoods(sdr_brow.GoodsID, &ti_pos) > 0)
										ilti.Cost = link_pack.TI(ti_pos).Cost;
								}
								{
									PPTransferItem ti(&pack.Rec, TISIGN_UNDEF);
									ti.SetupGoods(sdr_brow.GoodsID);
									ti.Cost  = ilti.Cost;
									ti.Price = ilti.Price;
									THROW(p_bobj->SetupImportedPrice(&pack, &ti, 0));
									ilti.Price = ti.Price;
								}
								//
								// ���������� ��������� �� �������������� ����
								//
								{
									ReceiptTbl::Rec last_lot;
									ReceiptTbl::Rec prev_lot;
									if(p_bobj->trfr->Rcpt.GetLastLot(sdr_brow.GoodsID, pack.Rec.LocID, pack.Rec.Dt, &last_lot) > 0) {
										if(p_bobj->trfr->Rcpt.GetPreviousLot(sdr_brow.GoodsID, pack.Rec.LocID, last_lot.Dt, last_lot.OprNo, &prev_lot) > 0)
											ilti.QCert = prev_lot.QCertID;
									}
								}
								ilti.Setup(sdr_brow.GoodsID, sign, sdr_brow.Qtty, ilti.Cost, ilti.Price);
								SETFLAG(ilti.Flags, PPTFR_RECEIPT, sign > 0);
								ilti.Expiry = sdr_brow.Expiry;
								if(p_bobj->ConvertILTI(&ilti, &pack, 0, CILTIF_INHLOTTAGS|CILTIF_ALLOWZPRICE, is_serial ? serial : (const char*)0) > 0) {
									// @v9.4.9 if(ilti.Rest != 0.0 && sign == -1) {
									if(sign == -1 && ilti.HasDeficit()) { // @v9.4.9
										if(pLog) {
											// @v9.4.9 PPObject::SetLastErrObj(PPOBJ_GOODS, labs(ilti.GoodsID));
											// @v9.4.9 PPSetError(PPERR_LOTRESTBOUND);
											PPSetObjError(PPERR_LOTRESTBOUND, PPOBJ_GOODS, labs(ilti.GoodsID)); // @v9.4.9
											pLog->LogLastError();
										}
										accept_doc = 0;
									}
								}
								else {
									CALLPTRMEMB(pLog, LogLastError());
									accept_doc = 0;
								}
							}
							else if(pLog) {
								// @v9.5.5 GetGoodsName(sdr_brow.GoodsID, goods_name);
								goods_obj.FetchNameR(sdr_brow.GoodsID, goods_name); // @v9.5.5
								// @v9.0.2 PPGetWord(PPWORD_GOODS, 0, buf);
								PPLoadString("ware", buf); // @v9.0.2
								(add_info = buf).CatBrackStr(goods_name).CatDiv(',', 2);
								PPLoadString("date", buf);
								add_info.Cat(buf).CatChar('[').Cat(bill_rec.Dt).CatChar(']').CatDiv(',', 2);
								// @v9.3.10 PPGetWord(PPWORD_BILLCODE, 0, buf.Z());
								PPLoadString("billno", buf); // @v9.3.10
								add_info.Cat(buf).CatBrackStr(bill_code);
								PPGetMessage(mfError, PPERR_ZEROQTTY, add_info, DS.CheckExtFlag(ECF_SYSSERVICE), buf.Z());
								buf.ShiftLeft();
								pLog->Log(buf);
							}
							ok = 1;
						}
						else if(pLog) {
							PPGetMessage(mfError, PPERR_BARCODENFOUND, serial, DS.CheckExtFlag(ECF_SYSSERVICE), buf);
							buf.ShiftLeft();
							pLog->Log(buf);
						}
					}
				}
			}
			if(accept_doc) {
				PPTransaction tra_inner(-1);
				THROW(tra_inner);
				// �������� ����� ��������� ��������������
				if(sdr_bill.OpID == StyloBhtIIConfig::oprkInventory) {
					if(bill_rec.ID) {
						InventoryTotal total;
						InventoryFilt filt;
						THROW(p_bobj->ExtractPacket(bill_rec.ID, &pack) > 0);
						filt.SetSingleBillID(bill_rec.ID);
						THROW(r_inv_tbl.CalcTotal(&filt, &total));
						pack.Rec.Amount = BR2(total.Amount);
						pack.Amounts.Put(PPAMT_MAIN, pack.Rec.CurID, total.Amount, 0, 1);
						THROW(p_bobj->TurnInventory(&pack, 0));
						DS.LogAction(PPACN_INVENTBYTERM, PPOBJ_BILL, bill_rec.ID, 0, 0);
					}
				}
				else {
					int    is_modif = 0;
					if(CheckOpFlags(pack.Rec.OpID, OPKF_NEEDVALUATION))
						p_bobj->AutoCalcPrices(&pack, 0, &is_modif);
					if(!is_modif)
						pack.InitAmounts();
					pack.Rec.Flags2 |= BILLF2_BHT; // @v7.2.0
					if(!p_bobj->TurnPacket(&pack, 0)) {
						CALLPTRMEMB(pLog, LogLastError());
					}
					else
						bill_rec = pack.Rec;
				}
				if(bill_rec.ID)
					if(!p_bobj->PutGuid(bill_rec.ID, &uuid, 0) && pLog)
						pLog->LogLastError();
				THROW(tra_inner.Commit());
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	ZDELETE(p_ie_bill);
	return ok;
}

// static
int SLAPI PPObjBHT::AddEBLineToPacket(PPBillPacket * pPack, const char * pBarcode, double qtty, double price, PPLogger * pLog)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	double rest = 0.0;
	Goods2Tbl::Rec g_rec;
	PPObjGoods g_obj;
	ReceiptTbl::Rec lot_rec;
	PUGL   deficit_list;
	PPObjTSession tses_obj;
	long   hdl_tses_iter = -1;
	SString msg_buf, fmt_buf;
	if(pPack && pBarcode) {
		MEMSZERO(g_rec);
		MEMSZERO(lot_rec);
		if(g_obj.SearchByBarcode(pBarcode, 0, &g_rec) > 0) {
			ILTI ilti;
			ilti.GoodsID  = g_rec.ID;
			ilti.SetQtty(-qtty);
			ilti.Price    = price;
			THROW(p_bobj->ConvertILTI(&ilti, pPack, 0, CILTIF_DEFAULT, 0));
			// @v9.4.9 if(R6(ilti.Rest) != 0)
			if(ilti.HasDeficit()) // @v9.4.9
				THROW(deficit_list.Add(&ilti, pPack->Rec.LocID, 0, pPack->Rec.Dt));
		}
		else if(p_bobj->SelectLotBySerial(pBarcode, 0, LConfig.Location, &lot_rec) > 0) {
			PPTransferItem ti;
			THROW(pPack->BoundsByLot(lot_rec.ID, 0, -1, &rest, 0));
			ti.Init(&pPack->Rec);
			THROW(ti.SetupGoods(lot_rec.GoodsID) > 0);
			if(pPack->IsDraft()) {
				THROW(ti.SetupLot(0, 0, 0));
				ti.Quantity_ = -qtty;
				THROW(pPack->LoadTItem(&ti, 0, pBarcode));
				ok = 1;
			}
			else {
				THROW(ti.SetupLot(lot_rec.ID, 0, 0));
				ti.Quantity_ = (qtty <= rest) ? -qtty : -rest;  // ���� ��������� ��� ��� ����,
				// ���� qtty > rest. � ���������� ����� ������ ���������.
				if(rest > 0.0) {
					ti.Price = (price > 0) ? price : ti.Price;
					THROW(pPack->LoadTItem(&ti, 0, 0));
					ok = 1;
				}
				if(pLog && qtty > rest) {
					char   stub[16];
					stub[0] = 0;
					const char * p_code = NZOR(pBarcode, stub);
					pLog->Log(msg_buf.Printf(PPLoadTextS(PPTXT_LOG_BHTLOWRESTBYLOT, fmt_buf), lot_rec.ID, p_code, rest, qtty));
				}
			}
		}
		else {
			if(pPack->IsDraft()) {
				SString tses_name;
				TSessLineTbl::Rec line_rec;
				tses_obj.P_Tbl->InitLineEnumBySerial(pBarcode, +1, &hdl_tses_iter);
				while(tses_obj.P_Tbl->NextLineEnum(hdl_tses_iter, &line_rec) > 0) {
					TSessionTbl::Rec tses_rec;
					if(tses_obj.Search(line_rec.TSessID, &tses_rec) > 0 &&
						oneof2(tses_rec.Status, TSESST_INPROCESS, TSESST_CLOSED)) {
						if(pLog) {
							tses_obj.MakeName(&tses_rec, tses_name);
							pLog->Log(msg_buf.Printf(PPLoadTextS(PPTXT_USEDSERIALFROMTSESS, fmt_buf), pBarcode, tses_name.cptr()));
						}
						PPTransferItem ti;
						ti.Init(&pPack->Rec);
						THROW(ti.SetupGoods(line_rec.GoodsID) > 0);
						THROW(ti.SetupLot(0, 0, 0));
						ti.Quantity_ = -fabs(qtty);
						THROW(pPack->LoadTItem(&ti, 0, pBarcode));
						ok = 1;
						break;
					}
				}
			}
			if(ok <= 0) {
				PPSetError(PPERR_BARCODEORSERNFOUND, pBarcode);
				CALLPTRMEMB(pLog, LogLastError());
			}
		}
	}
	if(pLog && deficit_list.getCount())
		deficit_list.Log(pLog);
	CATCHZOK
	tses_obj.P_Tbl->DestroyIter(hdl_tses_iter);
	return ok;
}

// static
int SLAPI PPObjBHT::AcceptExpendBillsPalm(const char * pHName, const char * pLName, const PPBhtTerminal * pBhtRec, PPLogger * pLog)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	const PPConfig & r_cfg = LConfig;
	PPID   op_id = 0;
	DbfTable ebill_tbl(pHName);
	DbfTable line_tbl(pLName);
	PPObjGoods g_obj;
	PPObjTSession tses_obj;
	{
		PPTransaction tra(1);
		THROW(tra);
		if(ebill_tbl.getNumRecs() && ebill_tbl.top() && line_tbl.getNumRecs()) {
			int fldn_id = 0, fldn_dt  = 0, fldn_inlocid = 0, fldn_outlocid = 0;
			int fldn_bid = 0, fldn_gid = 0, fldn_qtty = 0, fldn_price = 0, fldn_code = 0;

			ebill_tbl.getFieldNumber("ID",       &fldn_id);
			ebill_tbl.getFieldNumber("INLOCID",  &fldn_inlocid);
			ebill_tbl.getFieldNumber("OUTLOCID", &fldn_outlocid);
			ebill_tbl.getFieldNumber("DATE",     &fldn_dt);
			//
			line_tbl.getFieldNumber("BILLID",   &fldn_bid);
			line_tbl.getFieldNumber("GOODSID",  &fldn_gid);
			line_tbl.getFieldNumber("QUANTITY", &fldn_qtty);
			line_tbl.getFieldNumber("PRICE",    &fldn_price);
			line_tbl.getFieldNumber("CODE",     &fldn_code);
			do {
				int    r = 1;
				PPID   ebid = 0;
				PPID   in_loc_id = 0, out_loc_id = 0;
				DbfRecord eb_rec(&ebill_tbl);
				LDATE  dt = ZERODATE;
				BillTbl::Rec same_rec;
				PPBillPacket b_pack;

				ebill_tbl.getRec(&eb_rec);
				eb_rec.get(fldn_id, ebid);
				eb_rec.get(fldn_inlocid, in_loc_id);
				eb_rec.get(fldn_outlocid, out_loc_id);
				eb_rec.get(fldn_dt, dt);
				SETIFZ(dt, r_cfg.OperDate);
				if(in_loc_id && out_loc_id) {
					PPOprKind op_rec;
					op_id = pBhtRec->IntrExpndOpID;
					THROW_PP(GetOpData(op_id, &op_rec) > 0, PPERR_INTREXPNDNOTDEF);
					THROW_PP(oneof2(op_rec.OpTypeID, PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND), PPERR_INTREXPNDNOTDEF);
					THROW_PP(op_rec.AccSheetID == r_cfg.LocAccSheetID, PPERR_INTREXPNDNOTDEF);
				}
				else {
					op_id = pBhtRec->ExpendOpID;
					THROW_PP(op_id && oneof2(GetOpType(op_id), PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND), PPERR_INVEXPOP);
				}
				SETIFZ(in_loc_id, r_cfg.Location);
				THROW(b_pack.CreateBlank(op_id, 0, in_loc_id, 0));
				b_pack.Rec.LocID = in_loc_id;
				b_pack.Rec.Dt    = dt;
				ltoa(ebid, b_pack.Rec.Code, 10);
				if(out_loc_id) {
					if(!(b_pack.Rec.Object = PPObjLocation::WarehouseToObj(out_loc_id)))
						r = -1;
				}
				if(r > 0) {
					THROW(r = p_bobj->P_Tbl->SearchAnalog(&b_pack.Rec, BillCore::safDefault, 0, &same_rec));
					if(r > 0 && pLog) {
						SString msg_buf, bill_code;
						PPObjBill::MakeCodeString(&same_rec, 1, bill_code).Quot('(', ')');
						if(PPGetMessage(mfError, PPERR_DOC_ALREADY_EXISTS, bill_code, 1, msg_buf))
							pLog->Log(msg_buf);
					}
				}
				if(r < 0 && line_tbl.top()) {
					do {
						char   code[64];
						PPID   lebid = 0, gid = 0;
						double qtty = 0.0, price = 0.0;
						DbfRecord ebl_rec(&line_tbl);
						memzero(code, sizeof(code));
						line_tbl.getRec(&ebl_rec);
						ebl_rec.get(fldn_bid,   lebid);
						ebl_rec.get(fldn_gid,   gid);
						ebl_rec.get(fldn_qtty,  qtty);
						ebl_rec.get(fldn_price, price);
						ebl_rec.get(fldn_code,  code);
						strip(code);
						if(ebid == lebid && code[0] != '\0' && qtty > 0)
							THROW(AddEBLineToPacket(&b_pack, code, qtty, price, pLog));
					} while(line_tbl.next());
					b_pack.Rec.Flags2 |= BILLF2_BHT; // @v7.2.0
					THROW(r = p_bobj->__TurnPacket(&b_pack, 0, 1, 0));
					if(r > 0 && pLog)
						pLog->LogAcceptMsg(PPOBJ_BILL, b_pack.Rec.ID, 0);
					ok = 1;
				}
			} while(ebill_tbl.next());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
// } AHTOXA

// static
int SLAPI PPObjBHT::AcceptBills(const char * pHName, const char * pLName, PPLogger * pLog)
{
	int    ok = -1;
	PPObjGoodsGroup gg_obj;
	uint   bi, li;
	SString temp_buf, suppl_name;
	BhtFile bf_bill(pHName);
	BhtFile bf_line(pLName);
	BhtRecord br_bill;
	BhtRecord br_line;
	{
		PPTransaction tra(1);
		THROW(tra);
		THROW(bf_bill.InitRecord(&br_bill));
		THROW(bf_line.InitRecord(&br_line));
		for(bi = 0; bf_bill.EnumRecords(&bi, &br_bill) > 0;) {
			PPID   alt_grp_id = 0;
			char   bid[12], bdate[16];
			PPID   suppl_id = 0;
			br_bill.GetStr(0, bid, sizeof(bid));
			br_bill.GetStr(1, bdate, sizeof(bdate));
			br_bill.GetInt(2, &suppl_id);
			GetArticleName(suppl_id, suppl_name);
			for(li = 0; bf_line.EnumRecords(&li, &br_line) > 0;) {
				char   lbid[12], gid[16];
				br_line.GetStr(0, lbid, sizeof(lbid));
				br_line.GetStr(1, gid, sizeof(gid));
				if(stricmp(bid, lbid) == 0) {
					if(alt_grp_id == 0) {
						PPID   temp_id = 0;
						PPGoodsPacket gp;
						THROW(gg_obj.InitPacket(&gp, gpkndAltGroup, 0, 0, 0));
						temp_buf.Z().Cat(bid).Space().Cat(bdate).Space().Cat(suppl_name);
						STRNSCPY(gp.Rec.Name, temp_buf);
						if(gg_obj.SearchByName(gp.Rec.Name, &temp_id, 0) <= 0) {
							PPWaitMsg(gp.Rec.Name);
							THROW(gg_obj.PutPacket(&alt_grp_id, &gp, 0));
						}
						else
							break;
					}
					// @todo Process errors at next call
					gg_obj.AssignGoodsToAltGrp(atol(gid), alt_grp_id, 0, 0);
					ok = 1;
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI IdentifyGoods(PPObjGoods * pGObj, SString & rBarcode, PPID * pGoodsID, Goods2Tbl::Rec * pRec)
{
	int    ok = -1;
	PPID   goods_id = 0;
	if(rBarcode.C(0) == 'B') {
		int    wp = pGObj->GetConfig().IsWghtPrefix(rBarcode);
		if(wp && oneof2(rBarcode.Len(), 12, 13)) {
			SString buf, temp_buf;
			buf = rBarcode;
			buf.Trim(12);
			temp_buf = buf.Sub(7, buf.Len() - 7, temp_buf);
			// qtty = fdiv1000i(temp_buf.ToLong());
			temp_buf.Trim(7);
			goods_id = temp_buf.ShiftLeft(2).ToLong();
		}
		if(!goods_id) {
			PPObjBill * p_bobj = BillObj;
			PPIDArray lot_list;
			Goods2Tbl::Rec goods_rec;
			MEMSZERO(goods_rec);
			rBarcode.ShiftLeft();
			rBarcode.Trim(13);
			if(p_bobj->SearchLotsBySerial(rBarcode, &lot_list) > 0 && lot_list.getCount()) {
				ReceiptTbl::Rec lot_rec;
				if(p_bobj->trfr->Rcpt.Search(lot_list.at(0), &lot_rec) > 0)
					goods_id = lot_rec.GoodsID;
			}
			if(!goods_id && pGObj && pGObj->SearchByBarcode(rBarcode, 0, &goods_rec) > 0)
				goods_id = goods_rec.ID;
		}
	}
	else
		goods_id = atol(rBarcode);
	if(pGObj && pGObj->Fetch(goods_id, pRec) > 0)
		ok = 1;
	else
		goods_id = 0;
	ASSIGN_PTR(pGoodsID, goods_id);
	return ok;
}

// static
int SLAPI PPObjBHT::AcceptBillsToGBasket(const char * pHName, const char * pLName, PPLogger * pLog)
{
	struct BillRowEntry {
		long   BillID;
		long   GoodsID;
		char   Barcode[14];
		double Qtty;
		double Price;
		LDATE  Expiry;
	};
	SArray brows_list(sizeof(BillRowEntry));
	int    ok = -1, resume = 1;
	uint   bi, li;
	SString temp_buf, suppl_name;
	SString barcode;
	SString basket_name;
	ResolveGoodsItemList goods_list;
	BhtFile bf_bill(pHName);
	BhtFile bf_line(pLName);
	BhtRecord br_bill;
	BhtRecord br_line;
	PPObjGoods       g_obj;
	PPObjGoodsBasket gb_obj;

	THROW(bf_bill.InitRecord(&br_bill));
	THROW(bf_line.InitRecord(&br_line));
	//
	// �������� ������ ���������� � ������
	//
	for(li = 0; bf_line.EnumRecords(&li, &br_line) > 0;) {
		char   lbid[12], gid[16], str_price[16], expiry[16];
		PPID   goods_id = 0;
		Goods2Tbl::Rec goods_rec;
		double qtty = 0.0;
		double price = 0.0;
		ILTI   item;
		br_line.GetStr(0, lbid, sizeof(lbid));
		br_line.GetStr(1, gid, sizeof(gid));
		br_line.GetDbl(2, &qtty);
		br_line.GetStr(3, str_price, sizeof(str_price));
		br_line.GetStr(4, expiry, sizeof(expiry));
		barcode = gid;
		IdentifyGoods(&g_obj, barcode, &goods_id, &goods_rec);
		if(qtty != 0.0 && (goods_id || barcode.Len())) {
			price = atof(str_price);
			if(goods_id == 0) {
				ResolveGoodsItem gi(goods_id);
				barcode.CopyTo(gi.Barcode, sizeof(gi.Barcode));
				gi.Quantity = qtty;
				THROW_SL(goods_list.insert(&gi));
			}
			{
				BillRowEntry brow;
				brow.BillID  = atol(lbid);
				brow.GoodsID = goods_id;
				brow.Qtty    = qtty;
				brow.Price   = price;
				strtodate(expiry, DATF_DMY, &brow.Expiry);
				if(!checkdate(&brow.Expiry))
					brow.Expiry = ZERODATE;
				barcode.CopyTo(brow.Barcode, sizeof(brow.Barcode));
				THROW_SL(brows_list.insert(&brow));
				brows_list.sort(CMPF_LONG);
			}
		}
	}
	//
	// �������� ����������� ������, ���� ��� ����
	//
	PPWait(0);
	if(goods_list.getCount() && (resume = ResolveGoodsDlg(&goods_list, RESOLVEGF_SHOWBARCODE|RESOLVEGF_MAXLIKEGOODS|RESOLVEGF_SHOWEXTDLG)) > 0) {
		uint   goods_count = goods_list.getCount();
		for(uint i = 0; i < goods_count; i++) {
			const ResolveGoodsItem & r_goods_item = goods_list.at(i);
			int    remove = BIN(!r_goods_item.ResolvedGoodsID);
			if(r_goods_item.GoodsID) {
				uint pos = 0;
				while(brows_list.lsearch(&r_goods_item.GoodsID, &pos, CMPF_LONG, offsetof(BillRowEntry, GoodsID)) > 0) {
					if(remove) {
						THROW_SL(brows_list.atFree(pos));
					}
					else {
						((BillRowEntry*)brows_list.at(pos))->GoodsID = r_goods_item.ResolvedGoodsID;
						pos++;
					}
				}
			}
			else {
				uint pos = 0;
				while(brows_list.lsearch(r_goods_item.Barcode, &pos, PTR_CMPFUNC(Pchar), offsetof(BillRowEntry, Barcode)) > 0) {
					if(remove) {
						THROW_SL(brows_list.atFree(pos));
					}
					else {
						((BillRowEntry*)brows_list.at(pos))->GoodsID = r_goods_item.ResolvedGoodsID;
						pos++;
					}
				}
			}
		}
	}
	{
		PPTransaction tra(1);
		THROW(tra);
		PPWait(1);
		if(resume > 0 && brows_list.getCount()) {
			uint rows_count = brows_list.getCount();
			for(bi = 0; bf_bill.EnumRecords(&bi, &br_bill) > 0;) {
				int    gb_exists = 0, r = 0;
				char   bid[12], bdate[16];
				long   bills_num = 0;
				PPID   suppl_id = 0, gb_id = 0;
				PPBasketPacket gb_packet;
				br_bill.GetStr(0, bid, sizeof(bid));
				br_bill.GetStr(1, bdate, sizeof(bdate));
				br_bill.GetInt(2, &suppl_id);
				GetArticleName(suppl_id, suppl_name);
				temp_buf.Z().Cat(bid).Space().Cat(bdate).Space().Cat(suppl_name);
				basket_name = temp_buf;
				for(long basket_idx = 0; gb_obj.SearchByName(&gb_id, basket_name, &gb_packet) > 0; basket_idx++)
					(basket_name = temp_buf).Space().CatChar('#').Cat(basket_idx + 1);
				gb_packet.Init();
				gb_packet.Head.SupplID = suppl_id;
				STRNSCPY(gb_packet.Head.Name, basket_name);
				gb_id = 0;
				/* �������� ������ �������.
				THROW((r = gb_obj.SearchByName(&gb_id, temp_buf, &gb_packet)));
				if(r > 0) {
					if(PPObjGoodsBasket::IsLocked(gb_id))
						gb_exists = 1;
					else {
						THROW(gb_obj.PutPacket(&gb_id, 0, 0));
						gb_exists = 0;
						gb_id = 0;
						MEMSZERO(gb_packet.Head);
						gb_packet.Head.SupplID = suppl_id;
						STRNSCPY(gb_packet.Head.Name, temp_buf);
						gb_packet.GoodsID = 0;
						gb_packet.destroy();
					}
				}
				*/
				if(!gb_exists) {
					const  long bill_id = atol(bid);
					uint   row_pos = 0;
					if(brows_list.lsearch(&bill_id, &row_pos, CMPF_LONG) > 0) {
						BillRowEntry * p_brow = 0;
						for(;brows_list.enumItems(&row_pos, (void**)&p_brow) > 0 && p_brow->BillID == bill_id;) {
							uint   pos = 0;
							ILTI   item;
							item.GoodsID  = p_brow->GoodsID;
							item.Quantity = p_brow->Qtty;
							item.Price    = p_brow->Price;
							item.Expiry   = p_brow->Expiry;
							if(gb_packet.SearchGoodsID(item.GoodsID, &pos) > 0) {
								item.Quantity += gb_packet.Lots.at(pos).Quantity;
								gb_packet.DelItem(pos);
							}
							THROW(gb_packet.AddItem(&item, 0));
							bills_num++;
							ok = 1;
						}
						if(bills_num > 0)
							THROW(gb_obj.PutPacket(&gb_id, &gb_packet, 0));
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

//static
int SLAPI PPObjBHT::AcceptInvent(const char * pHName, const char * pLName, PPID opID, PPLogger * pLog)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	uint   bi, li;
	uint   line_counter = 0;
	uint   nr_bill = 0, nr_line = 0; // ���������� �������, ��������������, � ����� ���������� � �����
	SString msg_buf;
	SString barcode;
	PPBillPacket pack;
	BhtFile bf_bill(pHName);
	BhtFile bf_line(pLName);
	BhtRecord br_bill;
	BhtRecord br_line;
	InventoryCore & r_inv_tbl = p_bobj->GetInvT();
	PPObjGoods g_obj;
	PPLoadText(PPTXT_IMP_INVENTORYLINES, msg_buf);
	THROW_PP(opID, PPERR_INVOPNOTDEF);
	{
		PPTransaction tra(1);
		THROW(tra);
		bf_line.GetNumRecs(&nr_line);
		THROW(bf_bill.InitRecord(&br_bill));
		THROW(bf_line.InitRecord(&br_line));
		for(bi = 0; bf_bill.EnumRecords(&bi, &br_bill) > 0;) {
			LDATE  inv_dt;
			PPID   inv_id = 0;
			char   bid[12], bdate[16];
			PPObjBill::InvBlock inv_blk;
			br_bill.GetStr(0, bid, sizeof(bid));
			br_bill.GetStr(1, bdate, sizeof(bdate));
			strtodate(bdate, DATF_DMY, &inv_dt);
			SETIFZ(inv_dt, LConfig.OperDate);
			for(li = 0; bf_line.EnumRecords(&li, &br_line) > 0;) {
				char   lbid[32];
				char   bcode[24];
				long   goods_id = 0;
				double qtty = 0.0;
				br_line.GetStr(0, lbid,  sizeof(lbid));
				br_line.GetStr(1, bcode, sizeof(bcode));
				br_line.GetDbl(2, &qtty);
				if(stricmp(bid, lbid) == 0) {
					Goods2Tbl::Rec goods_rec;
					MEMSZERO(goods_rec);
					barcode = bcode;
					IdentifyGoods(&g_obj, barcode, &goods_id, &goods_rec);
					if(goods_id && qtty != 0.0) {
						if(!inv_id) {
							char   bill_code[32];
							STRNSCPY(bill_code, bid);
							strcat(bill_code, "B");
							DateIter di(inv_dt, inv_dt);
							BillTbl::Rec bill_rec;
							while(!inv_id && p_bobj->P_Tbl->EnumByOpr(opID, &di, &bill_rec) > 0)
								if(stricmp(bill_rec.Code, bill_code) == 0) {
									inv_id  = bill_rec.ID;
								}
							if(!inv_id) {
								THROW(pack.CreateBlank(opID, 0, 0, 0));
								STRNSCPY(pack.Rec.Code, bill_code);
								pack.Rec.Dt = inv_dt;
								THROW(p_bobj->TurnInventory(&pack, 0));
								inv_id  = pack.Rec.ID;
							}
							THROW(p_bobj->InitInventoryBlock(inv_id, inv_blk));
						}
						{
							PPObjBill::InvItem inv_item;
							inv_item.Init(goods_id, 0);
							inv_item.Qtty = qtty;
							if(!p_bobj->AcceptInventoryItem(inv_blk, &inv_item, 0)) {
								if(pLog)
									pLog->LogLastError();
								else {
									CALLEXCEPT();
								}
							}
							ok = 1;
						}
					}
					else if(pLog) {
						SString log_msg, fmt_buf, temp_buf;
						if(goods_id == 0) {
							temp_buf.Z().CatEq("InvNo", lbid);
							temp_buf.CatDiv(':', 1).CatEq("Dt", inv_dt);
							if(barcode.Len())
								temp_buf.CatDiv(':', 1).CatEq("Barcode", barcode);
							temp_buf.CatDiv(':', 1).CatEq("qtty", qtty);
							pLog->Log(log_msg.Printf(PPLoadTextS(PPTXT_LOG_IMPINV_GOODSNOTIDD, fmt_buf), temp_buf.cptr()));
						}
						else if(qtty <= 0.0) {
							temp_buf.Z().Cat(qtty, SFMT_QTTY);
							log_msg.Printf(PPLoadTextS(PPTXT_LOG_IMPINV_INVQTTY, fmt_buf), goods_rec.Name, temp_buf.cptr());
							pLog->Log(log_msg);
						}
					}
				}
				PPWaitPercent(line_counter, ++nr_line, msg_buf);
			}
			if(inv_id) {
				InventoryTotal total;
				InventoryFilt filt;
				THROW(p_bobj->ExtractPacket(inv_id, &pack) > 0);
				filt.SetSingleBillID(pack.Rec.ID);
				THROW(r_inv_tbl.CalcTotal(&filt, &total));
				pack.Rec.Amount = BR2(total.Amount);
				pack.Amounts.Put(PPAMT_MAIN, pack.Rec.CurID, total.Amount, 0, 1);
				THROW(p_bobj->TurnInventory(&pack, 0));
				DS.LogAction(PPACN_INVENTBYTERM, PPOBJ_BILL, inv_id, 0, 0);
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

// static
int SLAPI PPObjBHT::AcceptExpendBills(const char * pHName, const char * pLName, const PPBhtTerminal * pBhtRec, PPLogger * pLog)
{
	int    ok = -1, r = -1;
	PPObjBill * p_bobj = BillObj;
	char   op_id[24];
	uint   bi, li;
	BhtFile bf_bill(pHName);
	BhtFile bf_line(pLName);
	BhtRecord br_bill;
	BhtRecord br_line;
	PPOprKind  op_rec;
	PPObjGoods g_obj;
	MEMSZERO(op_rec);
	r = GetOpData(pBhtRec->ExpendOpID, &op_rec);
	PPSetAddedMsgString(longfmtz(op_rec.ID, 0, op_id, sizeof(op_id)));
	THROW_PP(r > 0 && op_rec.OpTypeID == PPOPT_GOODSEXPEND, PPERR_INVEXPOP)
	{
		PPTransaction tra(1);
		THROW(tra);
		THROW(bf_bill.InitRecord(&br_bill));
		THROW(bf_line.InitRecord(&br_line));
		for(bi = 0; bf_bill.EnumRecords(&bi, &br_bill) > 0;) {
			LDATE  be_dt = ZERODATE;
			char   beid[12], bdate[16];
			BillTbl::Rec same_rec;
			PPBillPacket b_pack;
			br_bill.GetStr(0, beid, sizeof(beid));
			br_bill.GetStr(1, bdate, sizeof(bdate));
			strtodate(bdate, DATF_DMY, &be_dt);
			if(!checkdate(be_dt))
				be_dt = getcurdate_();
			THROW(b_pack.CreateBlank(pBhtRec->ExpendOpID, 0, 0, 0))
			b_pack.Rec.Dt = be_dt;
			STRNSCPY(b_pack.Rec.Code, beid);
			THROW(r = p_bobj->P_Tbl->SearchAnalog(&b_pack.Rec, BillCore::safDefault, 0, &same_rec));
			if(r < 0) {
				for(li = 0; bf_line.EnumRecords(&li, &br_line) > 0;) {
					char   lbeid[12], barcode[14];
					PPID   lot_id = 0;
					double qtty = 0.0, count = 0.0;
					PUGL   deficit_list;
					br_line.GetStr(0, lbeid, sizeof(lbeid));
					br_line.GetStr(1, barcode, sizeof(barcode));
					br_line.GetDbl(2, &qtty);
					br_line.GetDbl(3, &count);
					if(stricmp(beid, lbeid) == 0 && barcode[0] != '\0' && count > 0) {
						AddEBLineToPacket(&b_pack, barcode, count, 0, pLog);
						ok = 1;
					}
				}
				b_pack.Rec.Flags2 |= BILLF2_BHT;
				THROW(p_bobj->__TurnPacket(&b_pack, 0, 1, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

#if 0 // @v9.4.9 {
static int SLAPI SaveFile(PPID fileID, const char * pDir, int removeSrc, int isWinCe = 0)
{
	int    ok = -1;
	if(fileID && pDir) {
		SString src_path = pDir;
		SString name;
		PPGetFileName(fileID, name);
		if(!isWinCe)
			src_path.SetLastSlash().Cat("out");
		src_path.SetLastSlash().Cat(name);
		if(isWinCe) {
			SPathStruc::ReplaceExt(src_path, "dbf", 1);
			src_path.Transf(CTRANSF_INNER_TO_OUTER);
		}
		if(fileExists(src_path)) {
			SString save_path;
			SPathStruc sps;
			PPGetPath(PPPATH_ROOT, save_path);
			save_path.SetLastSlash().Cat("Save").SetLastSlash();
			createDir(save_path);
			sps.Split(src_path);
			save_path.Cat(sps.Nam).Dot().Cat(sps.Ext);
			SCopyFile(src_path, save_path, 0, FILE_SHARE_READ, 0);
			if(removeSrc)
				SFile::Remove(src_path);
			ok = 1;
		}
	}
	return ok;
}

static int SLAPI CheckFile2(PPID fileID, const char * pDir, SStrCollection * pFiles, int * pFileDescr, PPID bhtTypeID = PPObjBHT::btPalm)
{
	int    ok = 0;
	SString name;
	SString path = pDir;
	PPGetFileName(fileID, name);
	if(bhtTypeID == PPObjBHT::btPalm)
		path.SetLastSlash().Cat("out");
	path.SetLastSlash().Cat(name);
	if(bhtTypeID == PPObjBHT::btWinCe)
		SPathStruc::ReplaceExt(path.Transf(CTRANSF_INNER_TO_OUTER), "dbf", 1);
	if(fileExists(path)) {
		int    descr = pFiles->getCount();
		if(bhtTypeID == PPObjBHT::btWinCe) {
			long   s = 1;
			SString new_dir, new_path;
			PPGetPath(PPPATH_IN, new_dir);
			MakeTempFileName(new_dir, "BHT", "DBF", &s, new_path);
			SCopyFile(path, new_path, 0, FILE_SHARE_READ, 0);
			path = new_path;
		}
		pFiles->insert(newStr(path));
		ASSIGN_PTR(pFileDescr, descr);
		ok = 1;
	}
	return ok;
}
#endif // } 0 @v9.4.9

// static
int SLAPI PPObjBHT::ReceiveData()
{
	class ReceiveData_LocalBlock {
	public:
		ReceiveData_LocalBlock(const PPBhtTerminalPacket & rPack, SStrCollection & rFiles) :
			R_Pack(rPack), R_Files(rFiles)
		{
		}
		int SLAPI CheckFile2(PPID fileID, int * pFileDescr, PPID bhtTypeID = PPObjBHT::btPalm)
		{
			int    ok = 0;
			SString name;
			SString path = R_Pack.ImpExpPath_;
			PPGetFileName(fileID, name);
			if(bhtTypeID == PPObjBHT::btPalm)
				path.SetLastSlash().Cat("out");
			path.SetLastSlash().Cat(name);
			if(bhtTypeID == PPObjBHT::btWinCe)
				SPathStruc::ReplaceExt(path.Transf(CTRANSF_INNER_TO_OUTER), "dbf", 1);
			if(fileExists(path)) {
				int    descr = R_Files.getCount();
				if(bhtTypeID == PPObjBHT::btWinCe) {
					long   s = 1;
					SString new_dir, new_path;
					PPGetPath(PPPATH_IN, new_dir);
					MakeTempFileName(new_dir, "BHT", "DBF", &s, new_path);
					SCopyFile(path, new_path, 0, FILE_SHARE_READ, 0);
					path = new_path;
				}
				R_Files.insert(newStr(path));
				ASSIGN_PTR(pFileDescr, descr);
				ok = 1;
			}
			return ok;
		}
		int SLAPI SaveFile(PPID fileID, int removeSrc, int isWinCe = 0)
		{
			int    ok = -1;
			if(fileID && R_Pack.ImpExpPath_.NotEmpty()) {
				SString src_path = R_Pack.ImpExpPath_;
				SString name;
				PPGetFileName(fileID, name);
				if(!isWinCe)
					src_path.SetLastSlash().Cat("out");
				src_path.SetLastSlash().Cat(name);
				if(isWinCe) {
					SPathStruc::ReplaceExt(src_path, "dbf", 1);
					src_path.Transf(CTRANSF_INNER_TO_OUTER);
				}
				if(fileExists(src_path)) {
					SString save_path;
					SPathStruc sps;
					PPGetPath(PPPATH_ROOT, save_path);
					save_path.SetLastSlash().Cat("Save").SetLastSlash();
					createDir(save_path);
					sps.Split(src_path);
					save_path.Cat(sps.Nam).Dot().Cat(sps.Ext);
					SCopyFile(src_path, save_path, 0, FILE_SHARE_READ, 0);
					if(removeSrc)
						SFile::Remove(src_path);
					ok = 1;
				}
			}
			return ok;
		}
	private:
		const PPBhtTerminalPacket & R_Pack;
		SStrCollection & R_Files;
	};
	int    ok = -1, r, valid_data = 0;
	int    fi_bill  = -1, fi_line   = -1;
	int    fi_inv   = -1, fi_iline  = -1;
	int    fi_ebill = -1, fi_ebline = -1;
	int    fi_tsline = -1;
	uint   i = 0;
	PPObjBHT bht_obj;
	PPID   bht_id = 0;
	PPID   dest_intr_loc_id = 0; // ����� ���������� ��� ���������� ���������� ��������
	PPBhtTerminalPacket pack;
	TDialog * dlg = new TDialog(DLG_BHTRCV);
	THROW(CheckDialogPtr(&dlg));
	bht_id = bht_obj.GetSingle();
	SetupPPObjCombo(dlg, CTLSEL_BHTRCV_BHT, PPOBJ_BHT, bht_id, 0);
	SetupLocationCombo(dlg, CTLSEL_BHTRCV_DINTRLOC, dest_intr_loc_id, 0, LOCTYP_WAREHOUSE, 0); // @v9.4.12
	for(valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		dlg->getCtrlData(CTLSEL_BHTSEND_BHT, &bht_id);
		dlg->getCtrlData(CTLSEL_BHTRCV_DINTRLOC, &dest_intr_loc_id);
		if(bht_id > 0)
			valid_data = 1;
	}
	ZDELETE(dlg);
	// @v9.4.9 if(valid_data && bht_obj.Search(bht_id, &bht_rec) > 0) {
	if(valid_data && bht_obj.GetPacket(bht_id, &pack) > 0) { // @v9.4.9
		int    is_debug = 0;
		long   s = 1, timeout = 30000L;
		SString dir, path;
		SString fn_bill;
		SString fn_bline;
		SString fn_invent;
		SString fn_iline;
		SString fn_ebill;
		SString fn_ebline;
		BhtProtocol bp;
		CipherProtocol cp;
		SStrCollection files;
		PPLogger logger;
		ReceiveData_LocalBlock _lb(pack, files);
		const   int bht_type = pack.Rec.BhtTypeID;
		THROW_PP(bht_type != PPObjBHT::btCom, PPERR_BHTSUPPFROMEXTMOD);
		PPWait(1);
		if(!oneof3(bht_type, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
			PPGetFileName(PPFILNAM_BHT_BILL,   fn_bill);
			PPGetFileName(PPFILNAM_BHT_BLINE,  fn_bline);
			PPGetFileName(PPFILNAM_BHT_INVENT, fn_invent);
			PPGetFileName(PPFILNAM_BHT_ILINE,  fn_iline);
			PPGetFileName(PPFILNAM_BHT_EBILL,  fn_ebill);
			PPGetFileName(PPFILNAM_BHT_EBLINE, fn_ebline);
		}
		if(bht_type == PPObjBHT::btDenso) {
			THROW(bht_obj.InitProtocol(bht_id, &bp));
		}
		else if(bht_type == PPObjBHT::btSyntech) {
			THROW(bht_obj.InitProtocol(bht_id, &cp));
		}
#ifdef NDEBUG
	is_debug = 0;
#endif
		if(is_debug) {
			PPGetPath(PPPATH_IN, dir);
			(path = dir.SetLastSlash()).Cat("bht?????.dat");
			SDirEntry sde;
			for(SDirec sd(path); sd.Next(&sde) > 0;)
				files.insert(newStr((path = dir).Cat(sde.FileName)));
		}
		else if(!oneof3(bht_type, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
			do {
				PPGetPath(PPPATH_IN, dir);
				MakeTempFileName(dir, "BHT", "DAT", &s, path);
				if(bht_type == PPObjBHT::btDenso) {
					THROW(r = bp.ReceiveFile(path, timeout));
				}
				else if(bht_type == PPObjBHT::btSyntech) {
					THROW(r = cp.ReceiveFile(path, timeout));
				}
				if(r > 0) {
					files.insert(newStr(path));
					timeout = 2000L;
					ok = 1;
				}
				else
					SFile::Remove(path);
			} while(r > 0);
		}
		else {
			if(bht_type == PPObjBHT::btPalm) {
				_lb.CheckFile2(PPFILNAM_BHTPALM_BILL,      &fi_bill);
				_lb.CheckFile2(PPFILNAM_BHTPALM_BITEM,     &fi_line);
				_lb.CheckFile2(PPFILNAM_BHTPALM_EBILL,     &fi_ebill);
				_lb.CheckFile2(PPFILNAM_BHTPALM_EBITEM,    &fi_ebline);
				_lb.CheckFile2(PPFILNAM_BHTPALM_INVENT,    &fi_inv);
				_lb.CheckFile2(PPFILNAM_BHTPALM_IITEM,     &fi_iline);
				_lb.CheckFile2(PPFILNAM_BHTPALM_TSESSITEM, &fi_tsline);
			}
			else {
				_lb.CheckFile2(PPFILNAM_BHT_BILL,      &fi_bill,  PPObjBHT::btWinCe);
				_lb.CheckFile2(PPFILNAM_BHT_BLINE,     &fi_line,  PPObjBHT::btWinCe);
				_lb.CheckFile2(PPFILNAM_BHT_INVENT,    &fi_inv,   PPObjBHT::btWinCe);
				_lb.CheckFile2(PPFILNAM_BHT_ILINE,     &fi_iline, PPObjBHT::btWinCe);
				_lb.CheckFile2(PPFILNAM_BHT_TSESSITEM, &fi_tsline, PPObjBHT::btWinCe);
			}
		}
		if(!oneof3(bht_type, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
			for(i = 0; i < files.getCount(); i++) {
				BhtFile bf(files.at(i));
				if(sstreqi_ascii(bf.Name, fn_bill))
					fi_bill = i;
				else if(sstreqi_ascii(bf.Name, fn_bline))
					fi_line = i;
				else if(sstreqi_ascii(bf.Name, fn_invent))
					fi_inv = i;
				else if(sstreqi_ascii(bf.Name, fn_iline))
					fi_iline = i;
				else if(sstreqi_ascii(bf.Name, fn_ebill))
					fi_ebill = i;
				else if(sstreqi_ascii(bf.Name, fn_ebline))
					fi_ebline = i;
			}
		}
		if(fi_bill >= 0 && fi_line >= 0) {
			const char * p_h_fname = files.at(fi_bill);
			const char * p_l_fname = files.at(fi_line);
			if(bht_type == PPObjBHT::btStyloBhtII) {
				THROW(AcceptBillsSBII(&pack, dest_intr_loc_id, p_h_fname, p_l_fname, &logger));
			}
			else if(pack.Rec.ReceiptPlace == RCPTPLACE_ALTGROUP) {
				if(!oneof2(bht_type, PPObjBHT::btPalm, PPObjBHT::btWinCe)) {
					THROW(AcceptBills(p_h_fname, p_l_fname, &logger));
				}
				else {
					THROW(AcceptBillsPalm(p_h_fname, p_l_fname, &logger));
				}
			}
			else if(pack.Rec.ReceiptPlace == RCPTPLACE_GBASKET) {
				if(!oneof2(bht_type, PPObjBHT::btPalm, PPObjBHT::btWinCe)) {
					THROW(AcceptBillsToGBasket(p_h_fname, p_l_fname, &logger));
				}
				else {
					THROW(AcceptBillsToGBasketPalm(p_h_fname, p_l_fname, &logger));
				}
			}
		}
		if(bht_type != PPObjBHT::btStyloBhtII && fi_inv >= 0 && fi_iline >= 0) {
			if(!oneof2(bht_type, PPObjBHT::btPalm, PPObjBHT::btWinCe)) {
				THROW(AcceptInvent(files.at(fi_inv), files.at(fi_iline), pack.Rec.InventOpID, &logger));
			}
			else {
				THROW(AcceptInventPalm(files.at(fi_inv), files.at(fi_iline), pack.Rec.InventOpID, &logger));
			}
		}
		if(bht_type != PPObjBHT::btStyloBhtII && fi_ebill >=0 && fi_ebline >=0) {
			if(!oneof2(bht_type, PPObjBHT::btPalm, PPObjBHT::btWinCe)) {
				THROW(AcceptExpendBills(files.at(fi_ebill), files.at(fi_ebline), &pack.Rec, &logger));
			}
			else {
				THROW(AcceptExpendBillsPalm(files.at(fi_ebill), files.at(fi_ebline), &pack.Rec, &logger));
			}
		}
		if(fi_tsline >= 0) {
			if(oneof2(bht_type, PPObjBHT::btPalm, PPObjBHT::btWinCe)) {
				THROW(AcceptTechSessPalm(files.at(fi_tsline), &logger));
			}
		}
		if(oneof3(bht_type, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
			if(bht_type == PPObjBHT::btPalm) {
				_lb.SaveFile(PPFILNAM_BHTPALM_BILL,      1);
				_lb.SaveFile(PPFILNAM_BHTPALM_BITEM,     1);
				_lb.SaveFile(PPFILNAM_BHTPALM_EBILL,     1);
				_lb.SaveFile(PPFILNAM_BHTPALM_EBITEM,    1);
				_lb.SaveFile(PPFILNAM_BHTPALM_INVENT,    1);
				_lb.SaveFile(PPFILNAM_BHTPALM_IITEM,     1);
				_lb.SaveFile(PPFILNAM_BHTPALM_TSESSITEM, 1);
			}
			else {
				const int del_src = BIN(pack.Rec.Flags & PPBhtTerminal::fDelAfterImport);
				_lb.SaveFile(PPFILNAM_BHT_BILL,      del_src, 1);
				_lb.SaveFile(PPFILNAM_BHT_BLINE,     del_src, 1);
				if(bht_type == PPObjBHT::btWinCe) {
					_lb.SaveFile(PPFILNAM_BHT_INVENT,    del_src, 1);
					_lb.SaveFile(PPFILNAM_BHT_ILINE,     del_src, 1);
					_lb.SaveFile(PPFILNAM_BHT_TSESSITEM, del_src, 1);
				}
			}
			for(uint i = 0; i < files.getCount(); i++)
				SFile::Remove(files.at(i));
		}
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}

static int SLAPI MakeDBFFilePath(const char * pDir, uint fnameID, SString & aPath)
{
	SString fname;
	PPGetFileName(fnameID, fname);
	SPathStruc::ReplaceExt(fname, "dbf", 1);
	(aPath = pDir).SetLastSlash().Cat(fname).Transf(CTRANSF_INNER_TO_OUTER);
	return 1;
}

int PPBhtTerminalPacket::ConvertToConfig(int expKind, StyloBhtIIConfig * pCfg) const
{
	int    ok = 1;
	if(P_SBIICfg) {
		SString db_symb;
		LDATETIME cur_dtm = getcurdatetime_();
		StyloBhtIIConfig sbii_cfg;
		CurDict->GetDbSymb(db_symb);
		if(oneof2(expKind, 0, 1))
			sbii_cfg.GoodsLastExch = cur_dtm;
		if(oneof2(expKind, 0, 2))
			sbii_cfg.ArticleLastExch = cur_dtm;
		if(oneof2(expKind, 0, 4))
			sbii_cfg.BillLastExch  = cur_dtm;
		P_SBIICfg->DeviceName.CopyTo(sbii_cfg.DeviceName, sizeof(sbii_cfg.DeviceName));
		sbii_cfg.Flags = P_SBIICfg->Flags;
		P_SBIICfg->WeightPrefix.CopyTo(sbii_cfg.WeightPrefix, sizeof(sbii_cfg.WeightPrefix));
		P_SBIICfg->QttyWeightPrefix.CopyTo(sbii_cfg.QttyWeightPrefix, sizeof(sbii_cfg.QttyWeightPrefix));
		P_SBIICfg->UserName.CopyTo(sbii_cfg.UserName, sizeof(sbii_cfg.UserName));
		P_SBIICfg->Password.CopyTo(sbii_cfg.Password, sizeof(sbii_cfg.Password));
		db_symb.CopyTo(sbii_cfg.DbSymb, sizeof(sbii_cfg.DbSymb));
		sbii_cfg.ServerAddr = P_SBIICfg->ServerAddr;
		sbii_cfg.ServerPort = P_SBIICfg->ServerPort;
		sbii_cfg.ServerMask = P_SBIICfg->ServerMask;
		sbii_cfg.DefQtty    = P_SBIICfg->DefQtty;
		sbii_cfg.DeviceID   = Rec.ID;
		{
			PPEquipConfig eq_cfg;
			if(ReadEquipConfig(&eq_cfg) > 0) {
				sbii_cfg.RngLimWgtGoods = eq_cfg.BHTRngLimWgtGoods;
				sbii_cfg.RngLimPrice    = eq_cfg.BHTRngLimPrice;
			}
		}
		ASSIGN_PTR(pCfg, sbii_cfg);
	}
	else
		ok = -1;
	return ok;
}

// static
int SLAPI PPObjBHT::TransmitData()
{
	class BhtSendDlg : public TDialog {
	public:
		BhtSendDlg(PPObjBHT * pObj) : TDialog(DLG_BHTSEND), P_BhtObj(pObj)
		{
			ushort v = 0;
			const  PPID   bht_id = pObj ? pObj->GetSingle() : 0;
			SetupPPObjCombo(this, CTLSEL_BHTSEND_BHT, PPOBJ_BHT, bht_id, 0);
			setCtrlData(CTL_BHTSEND_WHAT,  &v);
			setCtrlData(CTL_BHTSEND_FLAGS, &(v = 1));
			DisableCtrls();
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_BHTSEND_BHT)) {
				DisableCtrls();
				clearEvent(event);
			}
		}
		void   DisableCtrls()
		{
			const PPID bht_id = getCtrlLong(CTLSEL_BHTSEND_BHT);
			PPBhtTerminalPacket pack;
			if(P_BhtObj && bht_id && P_BhtObj->GetPacket(bht_id, &pack) > 0) {
				DisableClusterItem(CTL_BHTSEND_WHAT, 3, pack.Rec.BhtTypeID != PPObjBHT::btPalm);
				DisableClusterItem(CTL_BHTSEND_WHAT, 4, pack.Rec.BhtTypeID != PPObjBHT::btStyloBhtII);
				ushort v = getCtrlUInt16(CTL_BHTSEND_WHAT);
				if(pack.Rec.BhtTypeID != PPObjBHT::btPalm && v == 3)
					setCtrlData(CTL_BHTSEND_WHAT, &(v = 0));
				if(pack.Rec.BhtTypeID != PPObjBHT::btStyloBhtII && v == 4)
					setCtrlData(CTL_BHTSEND_WHAT, &(v = 0));
				disableCtrl(CTL_BHTSEND_FLAGS, oneof4(pack.Rec.BhtTypeID, PPObjBHT::btPalm, PPObjBHT::btWinCe, PPObjBHT::btCom, PPObjBHT::btStyloBhtII));
			}
		}
		PPObjBHT * P_BhtObj;
	};
	int    ok = -1, what = -1; // 0 - All, 1 - Goods, 2 - Suppl, 3 - Tech session
	int    force_update = 0;
	PPID   bht_id = 0;
	ushort v = 0;
	PPObjBHT bht_obj;
	PPBhtTerminal rec;
	BhtSendDlg * dlg = new BhtSendDlg(&bht_obj);
	THROW(CheckDialogPtr(&dlg));
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTLSEL_BHTSEND_BHT, &bht_id);
		if(bht_id) {
			dlg->getCtrlData(CTL_BHTSEND_WHAT, &v);
			what = v;
			dlg->getCtrlData(CTL_BHTSEND_FLAGS, &v);
			force_update = BIN(v);
		}
	}
	ZDELETE(dlg);
	if(what >= 0 && bht_obj.Search(bht_id, &rec) > 0) {
		if(rec.BhtTypeID == PPObjBHT::btDenso) {
			BhtProtocol bp;
			THROW(bht_obj.InitProtocol(bht_id, &bp));
			if(what == 0) {
				THROW(bht_obj.TransmitSuppl(&bp, force_update));
				THROW(bht_obj.TransmitGoods(bht_id, &bp, force_update));
			}
			else if(what == 1) {
				THROW(bht_obj.TransmitGoods(bht_id, &bp, force_update));
			}
			else if(what == 2) {
				THROW(bht_obj.TransmitSuppl(&bp, force_update));
			}
		}
		else if(rec.BhtTypeID == PPObjBHT::btSyntech) {
			CipherProtocol cp;
			THROW(bht_obj.InitProtocol(bht_id, &cp));
			if(what == 0) {
				THROW(bht_obj.TransmitSuppl(&cp, force_update));
				THROW(bht_obj.TransmitGoods(bht_id, &cp, force_update));
			}
			else if(what == 1) {
				THROW(bht_obj.TransmitGoods(bht_id, &cp, force_update));
			}
			else if(what == 2) {
				THROW(bht_obj.TransmitSuppl(&cp, force_update));
			}
		}
		else if(oneof2(rec.BhtTypeID, PPObjBHT::btWinCe, PPObjBHT::btStyloBhtII)) {
			const int is_sbii = BIN(rec.BhtTypeID == PPObjBHT::btStyloBhtII);
			SString suppl_path, goods_path;
			PPBhtTerminalPacket pack;
			THROW(bht_obj.GetPacket(bht_id, &pack));
			MakeDBFFilePath(pack.ImpExpPath_, PPFILNAM_BHT_SUPPL, suppl_path);
			MakeDBFFilePath(pack.ImpExpPath_, PPFILNAM_BHT_GOODS, goods_path);
			PPWait(1);
			if(what == 0) {
				PPIDArray addendum_goods_list;
				if(is_sbii) {
					THROW(bht_obj.PrepareBillData2(&pack, &addendum_goods_list));
					THROW(bht_obj.PrepareLocCellData(&pack));
				}
				THROW(bht_obj.PrepareSupplData(suppl_path, &pack));
				THROW(bht_obj.PrepareGoodsData(bht_id, goods_path, 0, rec.BhtTypeID, &addendum_goods_list));
			}
			else if(what == 1) {
				THROW(bht_obj.PrepareGoodsData(bht_id, goods_path, 0, rec.BhtTypeID, 0));
			}
			else if(what == 2) {
				THROW(bht_obj.PrepareSupplData(suppl_path, &pack));
			}
			else if(what == 4) {
				THROW(bht_obj.PrepareBillData2(&pack, 0));
			}
			if(is_sbii) {
				StyloBhtIIConfig sbii_cfg;
				pack.ConvertToConfig(what, &sbii_cfg);
				THROW(bht_obj.PrepareConfigData(&pack, &sbii_cfg));
			}
			PPWait(0);
		}
		else if(rec.BhtTypeID == PPObjBHT::btPalm) {
			SString goods_file, goods_path;
			SString suppl_file, suppl_path;
			SString loc_file, loc_path;
			SString tsess_file, tsess_path;
			PPBhtTerminalPacket pack;
			THROW(bht_obj.GetPacket(bht_id, &pack));
			PPGetFileName(PPFILNAM_BHTPALM_SUPPL,    suppl_file);
			PPGetFileName(PPFILNAM_BHTPALM_LOC,      loc_file);
			PPGetFileName(PPFILNAM_BHTPALM_GOODS,    goods_file);
			PPGetFileName(PPFILNAM_BHTPALM_TSESSHDR, tsess_file);
			(goods_path = pack.ImpExpPath_).SetLastSlash().Cat("in").SetLastSlash();
			(suppl_path = goods_path).Cat(suppl_file);
			(loc_path   = goods_path).Cat(loc_file);
			(tsess_path = goods_path).Cat(tsess_file);
			goods_path.Cat(goods_file);
			if(what == 0) {
				THROW(bht_obj.PrepareSupplData(suppl_path, &pack));
				THROW(bht_obj.PrepareLocData(loc_path, rec.BhtTypeID));
				THROW(bht_obj.PrepareGoodsData(bht_id, goods_path, 0, rec.BhtTypeID, 0));
				THROW(bht_obj.PrepareTechSessData(tsess_path, rec.BhtTypeID));
			}
			else if(what == 1) {
				THROW(bht_obj.PrepareGoodsData(bht_id, goods_path, 0, rec.BhtTypeID, 0));
			}
			else if(what == 2) {
				THROW(bht_obj.PrepareSupplData(suppl_path, &pack));
				THROW(bht_obj.PrepareLocData(loc_path, rec.BhtTypeID));
			}
			else if(what == 3) {
				THROW(bht_obj.PrepareTechSessData(tsess_path, rec.BhtTypeID));
			}
			PPWait(0);
		}
		else if(rec.BhtTypeID == PPObjBHT::btCom)
			THROW_PP(0, PPERR_BHTSUPPFROMEXTMOD);
	}
	CATCHZOKPPERR
	return ok;
}
//
// test com CPT720 dll
//
#ifdef TEST_CPT720_EXTMODULE // {

void DisplayError(HRESULT hr)
{
	char   buf[64];
	memzero(buf, sizeof(buf));
	if(hr == E_FAIL)
		STRNSCPY(buf, "Unknown error");
	else if(hr == CLASS_E_CLASSNOTAVAILABLE)
		STRNSCPY(buf, "Requested class not found");
	else if(hr == E_NOINTERFACE)
		STRNSCPY(buf, "Requested interface not found");
	else if(hr == E_OUTOFMEMORY)
		STRNSCPY(buf, "There is no memory");
	else if(hr == CLASS_E_NOAGGREGATION)
		STRNSCPY(buf, "Component does not support aggregation");
	else if(hr == CPT720_ERR_INITCOMPORT)
		STRNSCPY(buf, "Error of initialization com of port");
	else if(hr == CPT720_ERR_NOHANDSHAKEACK)
		STRNSCPY(buf, "Error of an establishment of connection with the terminal");
	else if(hr == CPT720_ERR_CLOSELINKFAULT)
		STRNSCPY(buf, "Mistake of break of connection with the terminal");
	else if(hr == CPT720_ERR_NOTSOHSTXSYMB)
		STRNSCPY(buf, "Mistake of reception of the data");
	else if(hr == CPT720_ERR_NOREPLY)
		STRNSCPY(buf, "The terminal does not answer transfer of the data");
	else if(hr == CPT720_ERR_NAK)
		STRNSCPY(buf, "The terminal has rejected reception of the data");
	else if(hr == CPT720_ERR_EOT)
		STRNSCPY(buf, "The terminal has interrupted reception of the data");
	else if(hr == CPT720_ERR_CANTOPENFILE)
		STRNSCPY(buf, "Can't open file");
	else if(hr == CPT720_ERR_DBFWRFAULT)
		STRNSCPY(buf, "Mistake of record of the data in DBF the table");
	else if(hr == CPT720_ERR_ERRCRTTBL)
		STRNSCPY(buf, "Mistake of creation DBF of the table");
	else if(hr == CPT720_ERR_DBFOPENFLT)
		STRNSCPY(buf, "Can't open DBF table");
	if(buf[0] != '\0')
		MessageBox(NULL, (LPCTSTR)buf, _T("CPT720 error"), MB_OK | MB_ICONERROR);
}

void CallbackSndFilePrctFunc(long prct, long total, const char * addedMsg, size_t addedMsgSize)
{
	char buf[256];
	strnzcpy(buf, addedMsg, addedMsgSize);
	PPWaitPercent(prct, total, buf);
}

int SLAPI CPT720Send()
{
	int    ok = 1;
	HRESULT hr;
	SmartPtr<CPT720Interface, &IID_CPT720Interface> s_p;
	PPWait(1);
	CoInitialize(NULL);
	THROW(SUCCEEDED(hr = s_p.CreateInstance(CLSID_CPT720, NULL, CLSCTX_ALL)));
	s_p->SetComPortParams(cbr19200, 8, 0, 0);
	s_p->SetComPortTimeouts(2000, 1, 400, 5, 300);
	s_p->SetTransmitParams(3000, 10);
	THROW(SUCCEEDED(hr = s_p->SendGoodsData(0, "h:\\ppy\\out\\ppgoods.dbf", 0)));
	THROW(SUCCEEDED(hr = s_p->SendSupplData(0, "h:\\ppy\\out\\ppsuppl.dbf", CallbackSndFilePrctFunc)));
	CATCH
		ok = 0;
		DisplayError(hr);
	ENDCATCH
	PPWait(0);
	s_p.Release();
	CoFreeUnusedLibraries();
	CoUninitialize();
	return ok;
}

int SLAPI CPT720Receive()
{
	int    ok = 1;
	HRESULT hr;
	SmartPtr<CPT720Interface, &IID_CPT720Interface> s_p;

	PPWait(1);
	CoInitialize(NULL);
	THROW(SUCCEEDED(hr = s_p.CreateInstance(CLSID_CPT720, NULL, CLSCTX_ALL)));
	s_p->SetComPortParams(cbr19200, 8, 0, 0);
	s_p->SetComPortTimeouts(2000, 1, 400, 5, 300);
	s_p->SetTransmitParams(3000, 10);
	THROW(SUCCEEDED(hr = s_p->ReceiveFiles(0, "h:\\ppy\\in\\bill.dbf", "h:\\ppy\\in\\bline.dbf",
		"h:\\ppy\\in\\inv.dbf", "h:\\ppy\\in\\iline.dbf")));
	CATCH
		ok = 0;
		DisplayError(hr);
	ENDCATCH
	PPWait(0);
	s_p.Release();
	CoFreeUnusedLibraries();
	CoUninitialize();
	return ok;
}

#endif // } TEST_CPT720_EXTMODULE
