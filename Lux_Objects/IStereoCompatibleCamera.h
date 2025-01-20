/******************************************************************************
* Copyright 2013 Autodesk, Inc.  All rights reserved. 
* This computer source code and related instructions and comments are the 
* unpublished confidential and proprietary information of Autodesk, Inc. and 
* are protected under applicable copyright and trade secret law.  They may not 
* be disclosed to, copied or used by any third party without the prior written 
* consent of Autodesk, Inc.
******************************************************************************/
#pragma once
#include <ifnpub.h>
#include <maxtypes.h>

class IStereoCameraCallback;

//! \brief Defines the interface ID for the IStereoCompatibleCamera interface.
/** The ID can be used to return the interface pointer through the GetInterface() method of classes which implement IStereoCompatibleCamera.
\code
Object* pCamObj = pCameraNode->GetObjectRef();
pCamObj = pCamObj?pCamObj->FindBaseObject():NULL;
if (pCamObj)
{
	IStereoCompatibleCamera* pInterface = static_cast<IStereoCompatibleCamera*>(pCamObj->GetInterface(IID_STEREO_COMPATIBLE_CAMERA));
}
return NULL;
\endcode
*/
#define IID_STEREO_COMPATIBLE_CAMERA Interface_ID(0x121a8cbc, 0xc775585)

///////////////////////////////////////////////////////////////////////////////
//
// class IStereoCompatibleCamera
//
//! \brief Camera plug-ins which implement this interface would be recognized and be supported by stereo camera system.
/*! If a camera plug-in wants to be supported by stereo camera system, it needs to implement this method and get recompiled.
*/
///////////////////////////////////////////////////////////////////////////////

class IStereoCompatibleCamera : public FPMixinInterface
{
	public:

	//! \brief Register a callback to get notified when certain changes of a camera occur.
	// ! And the camera plug-in should be responsible for calling the proper methods of the callback under the specific conditions.
	virtual void RegisterIStereoCompatibleCameraCallback(IStereoCameraCallback* callback) = 0;

	//! \brief UnRegister a previously registered callback.
	virtual void UnRegisterIStereoCompatibleCameraCallback(IStereoCameraCallback* callback) = 0;

	//! \brief Returns true if the camera should be created with a target node at time t, false otherwise.
	//! During creation of a stereo camera system, stereo camera system uses this method to decide if it need 
	//! create a system target node or not.
	//! \param[in] t		- The evaluation time
	virtual bool HasTargetNode(TimeValue t) const = 0;

	//! \brief Returns the camera's own target distance value at time t.
	// During creation of a stereo camera system, stereo camera system uses this method to decide the distance 
	//! of the system's target node at time t if "HasTargetNode(TimeValue t)" returns true.
	//! \param[in] t		- The evaluation time
	virtual float GetTargetDistance(TimeValue t) const = 0;

	//! \brief Returns a pointer to the interface metadata for this class.
	virtual FPInterfaceDesc* GetDesc() { return this->GetDescByID(IID_STEREO_COMPATIBLE_CAMERA); }
};

/** 
* A callback class for receiving notifications from camera classes which implement IStereoCompatibleCamera
* Stereo camera system implements this interface and during creation, stereo camera system would register it by calling 
* IStereoCompatibleCamera::RegisterIStereoCompatibleCameraCallback(IStereoCameraCallback* callback).
* After using it, stereo camera system would unregister it by calling IStereoCompatibleCamera::UnRegisterIStereoCompatibleCameraCallback(
* IStereoCameraCallback* callback).
* For any camera plug-in which wants to be supported by stereo camera system, it must call the proper methods of this interface under specific conditions.
*/
class IStereoCameraCallback : public MaxHeapOperators
{
public:
	virtual ~IStereoCameraCallback(){}

	/**
	* A camera plug-in which implements IStereoCompatibleCamera interface should call this method
	* when the camera is about to switch between target camera and free camera. For example, call it in the beginning of 
	* the GenCamera::SetType(int tp) call. If the returned "bHandled" is true, then it means stereo camera system
	* has already handled this event by creating/removing a target node for the whole system.
	* In this case, the camera plug-in should simply update its own "hasTarget" flag and "targetDistance" parameter 
	* without creating/removing a target node itself.
	* \param[in] the time when the change happens
	* \param[in] true if camera is about to be set as a target camera, false otherwise.
	* \param[inout] If "bTarget" is true, the desired distance of target node is passed in via this parameter. 
	* if "bTarget" is false, the distance of the system's target node will be returned via this parameter.
	* \return true if this notification has already been handled, false otherwise.
	*/
	virtual bool PreCameraTargetChanged(TimeValue t, bool bTarget, float& targetDistance) = 0;

	/**
	* A camera plug-in which implements IStereoCompatibleCamera interface should call this method
	* when the camera is about to set its target distance parameter. For example, call it in the beginning of the 
	* CameraObject::SetTDist(TimeValue t, float f) call. If the returned "bHandled" is true,
	* then it means stereo camera system has already handled this event by setting the distance of the system's target node.
	* In this case, the camera plug-in should simply set its own "targetDistance" parameter to the new value without moving a target node itself.
	* \param[in] the time when the change happens
	* \param[in] the new target distance
	* \return true if this notification has already been handled, false otherwise.
	*/
	virtual bool PreSetCameraTargetDistance(TimeValue t, float distance) = 0;

	/**
	* A camera plug-in which implements IStereoCompatibleCamera interface should call this method
	* when the camera is about to update its target distance parameter. For example, call it in the beginning of the 
	* CameraObject::UpdateTargDistance(TimeValue t, INode* inode) call.If If the returned "bHandled" is true,
	* then it means stereo camera system has already handled this event by updating its own target distance and return it via "distance" parameter.
	* In this case, the camera plug-in should simply use the returned "distance" value as its own updated target distance 
	* instead of calculating its own target distance.
	* \param[in] the time when the change happens
	* \param[out] the current distance from system node to the system's target node
	* \return true if this notification has already been handled, false otherwise.
	*/
	virtual bool PreCameraTargetDistanceUpdated(TimeValue t, float& distance) = 0;
};