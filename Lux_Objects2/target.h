//**************************************************************************/
// Copyright (c) 2015-2019 Luxrender.
// All rights reserved.
// 
//**************************************************************************/
// DESCRIPTION: Contains the Dll Entry stuff
// AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com www.3dfine.com
//***************************************************************************/

#ifndef __TARGET__H__ 

#define __TARGET__H__

class TargetObject: public GeomObject{			   
	friend class TargetObjectCreateCallBack;
	friend INT_PTR CALLBACK TargetParamDialogProc( HWND hDlg, UINT message, 
		WPARAM wParam, LPARAM lParam );
	
		// Mesh cache
		static HWND hSimpleCamParams;
		static IObjParam* iObjParams;		
		static Mesh mesh;		
		static int meshBuilt;

		void GetMat(TimeValue t, INode* inode, ViewExp& vpt, Matrix3& tm);
		void BuildMesh();

	//  inherited virtual methods for Reference-management
		RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message, BOOL propagate );

	public:
		TargetObject();
		virtual ~TargetObject();

		//  inherited virtual methods:

		// From BaseObject
		int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
		void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
		int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
		CreateMouseCallBack* GetCreateMouseCallBack();
		void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
		void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
		const TCHAR *GetObjectName(bool localized) { return GetString(IDS_TARGET); }

		// From Object
		ObjectState Eval(TimeValue time);
		void InitNodeName(TSTR& s) { s = GetString(IDS_TARGET); }
		int UsesWireColor() { return 1; }						// 6/19/01 2:26pm --MQM-- now we can set object color
		int IsRenderable() { return 0; }

		// From GeomObject
		int IntersectRay(TimeValue t, Ray& r, float& at);
		void GetWorldBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
		void GetLocalBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
		void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel );

		BOOL HasViewDependentBoundingBox() { return true; }

		// From Animatable 
		void DeleteThis() {
			 delete this; 
			 }
		Class_ID ClassID() { return Class_ID(TARGET_CLASS_ID,0); }  
		void GetClassName(TSTR& s, bool localized) { s = GetString(IDS_TARGET_CLASS); }
		int IsKeyable(){ return 1;}
		LRESULT CALLBACK TrackViewWinProc( HWND hwnd,  UINT message, 
	            WPARAM wParam,   LPARAM lParam ){return(0);}

		// From ref.h
		RefTargetHandle Clone(RemapDir& remap);

		// IO
		IOResult Save(ISave *isave);
		IOResult Load(ILoad *iload);
		
		void AddRenderitem(
			const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
			MaxSDK::Graphics::UpdateNodeContext& nodeContext,
			MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer);

		virtual unsigned long GetObjectDisplayRequirement() const;

		virtual bool PrepareDisplay(
			const MaxSDK::Graphics::UpdateDisplayContext& prepareDisplayContext);

		virtual bool UpdatePerNodeItems(
			const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
			MaxSDK::Graphics::UpdateNodeContext& nodeContext,
			MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer);

	};


#endif
