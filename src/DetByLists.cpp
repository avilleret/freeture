/*
								DetByLists.cpp

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*
*	This file is part of:	freeture
*
*	Copyright:		(C) 2014 Yoan Audureau -- FRIPON-GEOPS-UPSUD
*
*	License:		GNU General Public License
*
*	FreeTure is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*	FreeTure is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*	You should have received a copy of the GNU General Public License
*	along with FreeTure. If not, see <http://www.gnu.org/licenses/>.
*
*	Last modified:		20/10/2014
*
*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/**
 * @file    DetByLists.h
 * @author  Yoan Audureau -- FRIPON-GEOPS-UPSUD
 * @version 1.0
 * @date    17/06/2014
 */

#include "DetByLists.h"

struct evPathRes{

    bool success;
    string path;
};


Point roiPositionToFramePosition(Point roiPix, Point newOrigine, Point framePix){

    Point resPix = Point(0,0);

    if(roiPix.x > newOrigine.x){

        resPix.x = roiPix.x - newOrigine.x;

    }else if(roiPix.x < newOrigine.x){

        resPix.x = -(newOrigine.x - roiPix.x);

    }

    if(roiPix.y > newOrigine.y){

        resPix.y = -(roiPix.y - newOrigine.y);

    }else if(roiPix.y < newOrigine.y){

        resPix.y = newOrigine.y - roiPix.y;

    }

    framePix.x += resPix.x;
    framePix.y += resPix.y;

    return framePix;

}

Mat buildMeteorTrail( GlobalEvent &ev, int h, int w, bool originalIntensity){

    Mat event = Mat::zeros(h,w, CV_8UC1);

    vector<LocalEvent>::iterator it;

    vector<LocalEvent> *p = ev.getListLocalEvent();

    for (it=p->begin(); it!=p->end(); ++it){

        LocalEvent &e = *it;

        vector< vector<PixelEvent> >::iterator it1;

        for (it1=e.listRoiPix.begin(); it1!=e.listRoiPix.end(); ++it1){

            vector<PixelEvent> &vpe = *it1;

            vector<PixelEvent>::iterator it2;

            for (it2=vpe.begin(); it2!=vpe.end(); ++it2){

                PixelEvent &pe = *it2;

               // cout << "val: "<< pe.intensity<<endl;

                if(originalIntensity)
                    event.at<uchar>(pe.position.y, pe.position.x) = (unsigned char)pe.intensity;
                else
                    event.at<uchar>(pe.position.y, pe.position.x) = 255;
            }
        }
    }

    return event;

}

vector<PixelEvent> createListPixSupThreshold(Mat &frame, const int *roiSize,Point framePixPos){

    src::severity_logger< severity_level > log;


    Mat roi;
     vector<PixelEvent> listPix;

   // if(pixelFormat == 8){

        unsigned char * ptr;

        frame(Rect(framePixPos.x-roiSize[0]/2,framePixPos.y-roiSize[1]/2,roiSize[0],roiSize[1])).copyTo(roi);

        // loop roi
        for(int i = 0; i < roi.rows; i++){

            ptr = roi.ptr<unsigned char>(i);


            for(int j = 0; j < roi.cols; j++){

                if(ptr[j] > 0){


                        PixelEvent *pix = new PixelEvent(roiPositionToFramePosition(Point(j, i), Point(roiSize[0]/2, roiSize[1]/2), framePixPos), ptr[j]);

                        listPix.push_back(*pix);

                }
            }
        }

      //  cout << "Nb Pix > seuil in ROI : "<<listPixSupThresh.size()<<endl;

  /*  }else{


        unsigned short * ptr;

        frame(Rect(framePixPos.x-roiSize[0]/2,framePixPos.y-roiSize[1]/2,roiSize[0],roiSize[1])).copyTo(roi);

        // loop roi
        for(int i = 0; i < roi.rows; i++){

            ptr = roi.ptr<unsigned short>(i);


            for(int j = 0; j < roi.cols; j++){

                if(ptr[j] > 0){

                    PixelEvent *pix = new PixelEvent(roiPositionToFramePosition(Point(j, i), Point(roiSize[0]/2, roiSize[1]/2), framePixPos), ptr[j]);

                    listPix.push_back(*pix);

                }
            }
        }

    }
*/





    return listPix;
}

vector<Scalar> getColorInEventMap(Mat &eventMap, Point framePixPos, const int *roiSize ){

    Mat roi;

    // get roi in eventmap
    eventMap(Rect(framePixPos.x-roiSize[0]/2, framePixPos.y-roiSize[1]/2, roiSize[0], roiSize[1])).copyTo(roi);

    unsigned char *ptr = (unsigned char*)roi.data;

    int cn = roi.channels();

    vector<Scalar> listColor;

    bool alreadyExist =false;

    for(int i = 0; i < roi.rows; i++){

        for(int j = 0; j < roi.cols; j++){

            Scalar bgrPixel;
            bgrPixel.val[0] = ptr[i*roi.cols*cn + j*cn + 0]; // B
            bgrPixel.val[1] = ptr[i*roi.cols*cn + j*cn + 1]; // G
            bgrPixel.val[2] = ptr[i*roi.cols*cn + j*cn + 2]; // R

            // si un pixel du roi n'est pas noir alors ce roi est rattaché à un groupe existant
            if(bgrPixel.val[0] != 0 || bgrPixel.val[1] != 0 || bgrPixel.val[2] != 0){

                for(int k = 0; k< listColor.size(); k++){

                    if(bgrPixel == listColor.at(k)){
                        alreadyExist = true;
                        break;

                    }

                }

                if(!alreadyExist)
                    listColor.push_back(bgrPixel);

                alreadyExist =false;

            }
        }
    }

    if(listColor.size() == 0)
        listColor.push_back(Scalar(0,0,0));

    return listColor;

}

void buildRecEvent(GlobalEvent &gEvent, string recPath, vector<RecEvent> &listRecEvent, boost::mutex &m_listRecEvent){

  //GlobalEvent newGlobalEvent((*itLE),(*itLE).getMap(),date);
    RecEvent rec = gEvent.extractEventRecInfos();

    rec.setPath(recPath);

    boost::mutex::scoped_lock lock_listRecEvent(m_listRecEvent);

    listRecEvent.push_back(rec);

    lock_listRecEvent.unlock();

}

template <typename Container> void stringtok_(Container &container, string const &in, const char * const delimiters = "_"){

    const string::size_type len = in.length();
    string::size_type i = 0;

    while (i < len){

        // Eat leading whitespace
        i = in.find_first_not_of(delimiters, i);

        if (i == string::npos)

            return;   // Nothing left but white space

        // Find the end of the token
        string::size_type j = in.find_first_of(delimiters, i);

        // Push token
        if (j == string::npos){

            container.push_back(in.substr(i));

            return;

        }else

            container.push_back(in.substr(i, j-i));

        // Set up for next loop
        i = j + 1;

    }
}

//Build record location with the following template : /STATION_DD-MM-AA/event/HH_UT/ev_station_date_hms/
evPathRes buildRecEventLocation( GlobalEvent &gEvent, string recPath, string stationName ){

    namespace fs = boost::filesystem;

    struct evPathRes functReturn;

    src::severity_logger< severity_level > log;

    functReturn.path = "";
    functReturn.success = false;

    //STATION_AAMMDD
    string root = recPath + stationName + "_" + gEvent.getDate().at(0) + gEvent.getDate().at(1) + gEvent.getDate().at(2) +"/";

    //event
    string sub0 = "events/";

    //STATION_AAAAMMDD_HHMMSS_UT_version
    string sub1 = stationName + "_" + gEvent.getDate().at(0)
                                           + gEvent.getDate().at(1)
                                           + gEvent.getDate().at(2) + "_"
                                           + gEvent.getDate().at(3)
                                           + gEvent.getDate().at(4)
                                           + gEvent.getDate().at(5) + "_UT-"
                                           + Conversion::intToString(0) + "/";
    //Data location/
    path p(recPath);

    //Data location/ + STATION_AAMMDD/
    path p0(root);

    //Data location/ + STATION_AAMMDD/ + events/
    string path1 = root + sub0;
    path p1(path1);

    //Data location/ + STATION_AAMMDD/ + events/ + STATION_AAAAMMDD_HHMMSS_UT_version/
    string path2 = root + sub0 + sub1;
    path p2(path2);

    //If Data location exists
    if(fs::exists(p)){

        //If Data location/ + STATION_AAMMDD/ already exists
        if(fs::exists(p0)){

            BOOST_LOG_SEV(log,notification) << "Destination directory " << p0.string() << " already exists.";

            //If Data location/ + STATION_AAMMDD/ + events/  already exists
            if(fs::exists(p1)){

                BOOST_LOG_SEV(log,notification) << "Destination directory " << p1.string() << " already exists.";

                if(fs::exists(p2)){

                    BOOST_LOG_SEV(log,notification) << "Destination directory " << p2.string() << " already exists.";

                    bool createFlag = false;
                    int version = 0;

                    string finalPath = "";

                    do{

                        finalPath = path2 + stationName + "_" + gEvent.getDate().at(0)
                                                   + gEvent.getDate().at(1)
                                                   + gEvent.getDate().at(2) + "_"
                                                   + gEvent.getDate().at(3)
                                                   + gEvent.getDate().at(4)
                                                   + gEvent.getDate().at(5) + "_UT-"
                                                   + Conversion::intToString(version) + "/";
                        path p5(finalPath);

                        // Create the destination directory
                        if(!fs::create_directory(p5)){

                            //std::cerr << "Unable to create destination directory" << p5.string() << '\n';
                            BOOST_LOG_SEV(log,notification) << "Unable to create destination directory" << p5.string();
                            version ++;

                        }else{
                            //std::cout << "Success to create directory : " << p5.string() << '\n';
                            BOOST_LOG_SEV(log,notification) << "Success to create directory : " << p5.string() ;
                            createFlag = true;

                        }

                    }while(!createFlag);

                    if(createFlag){

                        functReturn.success = true;
                        functReturn.path = finalPath;
                        return functReturn;

                    }

                }else{

                    if(!fs::create_directory(p2)){

                        BOOST_LOG_SEV(log,notification) << "Unable to create destination directory" << p2.string();
                        return functReturn;

                    }else{

                        functReturn.success = true;
                        functReturn.path = path2;
                        return functReturn;
                    }

                }

            }else{

                // Create the destination directory
                if(!fs::create_directory(p1)){
                    //std::cerr << "Unable to create destination directory" << p1.string() << '\n';
                    BOOST_LOG_SEV(log,notification) << "Unable to create destination directory" << p1.string();
                    return functReturn;

                }else{

                    // Create the destination directory
                    if(!fs::create_directory(p2)){
                        //std::cerr << "Unable to create destination directory" << p3.string() << '\n';
                        BOOST_LOG_SEV(log,notification) << "Unable to create destination directory" << p2.string();
                        return functReturn;

                    }else{

                        functReturn.success = true;
                        functReturn.path = path2;
                        return functReturn;

                    }

                }
            }

        }else{

            // Create the destination directory
            if(!fs::create_directory(p0)){

                //std::cerr << "Unable to create destination directory" << p0.string() << '\n';
                BOOST_LOG_SEV(log,notification) << "Unable to create destination directory" << p0.string();
                return functReturn;


            }else{

                if(!fs::create_directory(p1)){
                    //std::cerr << "Unable to create destination directory" << p1.string() << '\n';
                    BOOST_LOG_SEV(log,notification) << "Unable to create destination directory" << p1.string();
                    return functReturn;

                }else{

                    if(!fs::create_directory(p2)){
                        //std::cerr << "Unable to create destination directory" << p3.string() << '\n';
                        BOOST_LOG_SEV(log,notification) << "Unable to create destination directory" << p2.string();
                        return functReturn;


                    }else{

                        functReturn.success = true;
                        functReturn.path = path2;
                        return functReturn;

                    }

                }
            }
        }

    }else{

        if(!fs::create_directory(p)){

            BOOST_LOG_SEV(log,notification) << "Unable to create destination directory" << p.string();
            return functReturn;

        }else{

            // Create the destination directory
            if(!fs::create_directory(p0)){

                BOOST_LOG_SEV(log,notification) << "Unable to create destination directory" << p0.string();
                return functReturn;

            }else{

                if(!fs::create_directory(p1)){

                    BOOST_LOG_SEV(log,notification) << "Unable to create destination directory" << p1.string();
                    return functReturn;

                }else{

                    if(!fs::create_directory(p2)){

                        BOOST_LOG_SEV(log,notification) << "Unable to create destination directory" << p2.string();
                        return functReturn;


                    }else{

                        functReturn.success = true;
                        functReturn.path = path2;
                        return functReturn;

                    }
                }
            }
        }
    }
}

int defineThreshold(Mat i, Mat &mask){

    Scalar imgMean;
    Scalar imgStddev;
    Mat newMask;
    mask.convertTo(newMask, CV_16UC1);

    meanStdDev(i, imgMean, imgStddev, mask);

    cout << "stdev : " << 5*(int)(cvRound(imgStddev.val[0])+1) << endl;

    return 5*(int)(cvRound(imgStddev.val[0])+1);

}

void extractGERegion(vector<GlobalEvent> &listGE, vector<Point> &areaAroundGE, int areaSize, int imgW, int imgH){

    vector<GlobalEvent>::iterator itGE;
    src::severity_logger< severity_level > log;

    // Create ROI around existing GE
    for (itGE = listGE.begin(); itGE!=listGE.end(); ++itGE){

        Point pos = (*itGE).getListLocalEvent()->back().centerOfMass;
        BOOST_LOG_SEV(log,notification) << " GE position : " << pos;

        int x = pos.x - areaSize/2, y = pos.y - areaSize/2;
        int w = areaSize, h = areaSize;

        //BOOST_LOG_SEV(log,notification) << " x : " << x << " y : " << y;
        //BOOST_LOG_SEV(log,notification) << " w : " << w << " h : " << h;

        if(x < 0){

            x = 0;
            w = pos.x + areaSize/2;

        }

        if(y < 0){

            y = 0;
            h = pos.y + areaSize/2;

        }

        if((pos.x + areaSize/2 ) > imgW){

            w = imgW - pos.x + areaSize/2;

        }

        if((pos.y + areaSize/2) > imgH){

            h = imgH - pos.y + areaSize/2;

        }

       // BOOST_LOG_SEV(log,notification) << " x : " << x << " y : " << y;
        //BOOST_LOG_SEV(log,notification) << " w : " << w << " h : " << h;

        areaAroundGE.push_back(Point(x,y));
        areaAroundGE.push_back(Point(w,h));

    }

}

void searchROI( Mat &area,                      //Region where to search ROI
                Mat &frame,                     //Original image
                Mat maskNeighborhood,
                Mat &eventMap,
                Scalar &groupColor,
                vector<LocalEvent> &listLE,     //List of local event
                int imgH,                       //Original image height
                int imgW,                       //Original image width
                int const *roiSize,
                int areaPosX,                   //Size of region where to search ROI
                int areaPosY,                   //Size of region where to search ROI
                Point areaPosition,
                int pixelFormat ){           //Position of the region

    src::severity_logger< severity_level > log;

   // if(pixelFormat == 8){

        unsigned char * ptr;

        //height
        for(int i = 0; i < area.rows; i++){

            ptr = area.ptr<unsigned char>(i);

            //width
            for(int j = 0; j < area.cols; j++){

                // Current pixel value > threshold
                if((int)ptr[j] > 0){


                    // Check if ROI is not out of range in the frame
                    if((areaPosition.y + i - roiSize[1]/2 > 0) && (areaPosition.y + i + roiSize[1]/2 < imgH) && ( areaPosition.x + j-roiSize[0]/2 > 0) && (areaPosition.x + j + roiSize[0]/2 < imgW)){

                       // Mat testNeighborhood = frame(Rect(areaPosition.x + j - 1,areaPosition.y + i -1,3,3));

                      //  Mat resTest = testNeighborhood & maskNeighborhood;

                        //int nbPixNonZero = countNonZero(resTest);

                        //if(nbPixNonZero > 1){

                            vector<PixelEvent> listPixInRoi = createListPixSupThreshold(frame, roiSize, Point(areaPosition.x + j, areaPosition.y + i)/*, pixelFormat*/);

                            //Get color in ROI's eventMap
                            vector<Scalar> listColorInRoi = getColorInEventMap(eventMap, Point(areaPosition.x + j, areaPosition.y + i), roiSize );

                            if(listColorInRoi.size() == 1){

                                // If there is an other color than black
                                if( listColorInRoi.at(0).val[0]!=0 || listColorInRoi.at(0).val[1] != 0 || listColorInRoi.at(0).val[2] != 0 ){


                                    vector<LocalEvent>::iterator it;

                                    for (it=listLE.begin(); it!=listLE.end(); ++it){

                                        // If an existing localEvent matches
                                       if((*it).getColor() == listColorInRoi.at(0)){

                                          //   if((*it).listRoiCenter.size() < 10){
//
                                                // Merge localEvents
                                               (*it).listRoiCenter.push_back(Point(areaPosition.x + j, areaPosition.y + i));

                                                Mat tempMat = (*it).getMap();

                                                Mat roiTemp(roiSize[1],roiSize[0],CV_8UC1,Scalar(255));

                                                roiTemp.copyTo(tempMat(Rect(areaPosition.x + j - roiSize[0]/2, areaPosition.y + i - roiSize[1]/2, roiSize[0], roiSize[1])));

                                                (*it).setMap(tempMat);

                                                (*it).listRoiPix.push_back(listPixInRoi);

                                                // Update center of mass
                                                (*it).computeCenterOfMass(false);

                                             // }

                                        }else if((*it).listColor.size()!=0){

                                            for(int l = 0; l< (*it).listColor.size();l++){

                                                if((*it).listColor.at(l) == listColorInRoi.at(0)){


                                                //    if((*it).listRoiCenter.size() < 10){

                                                        // Merge localEvents
                                                       (*it).listRoiCenter.push_back(Point(areaPosition.x + j, areaPosition.y + i));

                                                        Mat tempMat = (*it).getMap();

                                                        Mat roiTemp(roiSize[1],roiSize[0],CV_8UC1,Scalar(255));

                                                        roiTemp.copyTo(tempMat(Rect(areaPosition.x + j - roiSize[0]/2, areaPosition.y + i - roiSize[1]/2, roiSize[0], roiSize[1])));

                                                        (*it).setMap(tempMat);

                                                        (*it).listRoiPix.push_back(listPixInRoi);


                                                        // Update center of mass
                                                        (*it).computeCenterOfMass(false);

                                                    }

                                               // }
                                            }


                                        }
                                    }



                                   // Color eventdMap's ROI with the color
                                   Mat roi(roiSize[1],roiSize[0],CV_8UC3,listColorInRoi.at(0));
                                   roi.copyTo(eventMap(Rect( areaPosition.x + j-roiSize[0]/2, areaPosition.y + i-roiSize[1]/2,roiSize[0],roiSize[1])));

                                }else{

                                    //BOOST_LOG_SEV(log,notification) << "Create new localEvent";

                                    // Compute new color group
                                    if(groupColor.val[0] + 40 <= 250){

                                        groupColor.val[0] += 40;  // B

                                    }else if(groupColor.val[1]  + 30 <= 250){

                                        groupColor.val[1] += 30;  // G

                                    }else if(groupColor.val[2] + 30 <= 250){

                                        groupColor.val[2] += 30;  // R

                                    }

                                    // Create new localEvent object
                                    LocalEvent newLocalEvent(groupColor, Point(areaPosition.x + j, areaPosition.y + i), listPixInRoi, imgH, imgW, roiSize);
                                    newLocalEvent.computeCenterOfMass(false);

                                    // And add it in the list of localEvent
                                    listLE.push_back(newLocalEvent);

                                    // Update eventMap with the color of the new localEvent group
                                    //BOOST_LOG_SEV(log,notification) << " Update eventMap";
                                    Mat roi(roiSize[1], roiSize[0], CV_8UC3, groupColor);
                                    roi.copyTo(eventMap(Rect( areaPosition.x + j-roiSize[0]/2, areaPosition.y + i-roiSize[1]/2,roiSize[0],roiSize[1])));

                                }
                                //BOOST_LOG_SEV(log,notification) << " Prepare to color in black";
                                int height = roiSize[1];
                                int width = roiSize[0];
                                int posX = j - roiSize[0]/2;
                                int posY = i - roiSize[1]/2;

                                if(j - roiSize[0]/2 < 0){

                                    width = j + roiSize[0]/2;
                                    posX = 0;

                                }else if(j + roiSize[0]/2 > areaPosX){

                                    width = areaPosX - j + roiSize[0]/2;

                                }

                                if(i - roiSize[1]/2 < 0){

                                    height = i + roiSize[1];
                                    posY = 0;


                                }else if(i + roiSize[1]/2 > areaPosY){

                                    height = areaPosY - i + roiSize[0]/2;

                                }

                                //BOOST_LOG_SEV(log,notification) << " Color in black region";

                                Mat roiBlackRegion(height,width,CV_8UC1,Scalar(0));
                                roiBlackRegion.copyTo(area(Rect(posX, posY, width, height)));

                                //BOOST_LOG_SEV(log,notification) << " Color in black diff";
                                Mat roiBlack(roiSize[1],roiSize[0],CV_8UC1,Scalar(0));
                                roiBlack.copyTo(frame(Rect( areaPosition.x + j-roiSize[0]/2, areaPosition.y + i-roiSize[1]/2,roiSize[0],roiSize[1])));


                            }else{



                                vector<LocalEvent>::iterator itRmLE;

                                vector<Scalar>::iterator it2;
                                vector<LocalEvent>::iterator it;

                                itRmLE = listLE.begin();
                                it2 = listColorInRoi.begin();

                                bool firstLE = false;

                                bool flag =false;
                                bool flag1 =false;
                                bool exit = false;

                                vector<Scalar> listLEtoRM;

                                Scalar mainColor;

                                // Trouver un premier LE qui a une couleur du ROI
                                while ( itRmLE != listLE.end() ){

                                     while ( it2 != listColorInRoi.end() ){

                                        // If an existing localEvent matches
                                        if( (*itRmLE).getColor() == (*it2)){

                                            if(!firstLE){

                                                it = itRmLE;

                                                firstLE = true;

                                                (*itRmLE).listRoiCenter.push_back(Point(areaPosition.x + j, areaPosition.y + i));

                                                Mat tempMat = (*itRmLE).getMap();

                                                Mat roiTemp(roiSize[1],roiSize[0],CV_8UC1,Scalar(255));

                                                roiTemp.copyTo(tempMat(Rect(areaPosition.x + j - roiSize[0]/2, areaPosition.y + i - roiSize[1]/2, roiSize[0], roiSize[1])));

                                                (*itRmLE).setMap(tempMat);

                                                (*itRmLE).listRoiPix.push_back(listPixInRoi);

                                                // Update center of mass
                                                (*itRmLE).computeCenterOfMass(false);

                                                mainColor = (*it2);

                                                it2 = listColorInRoi.erase(it2);


                                            }else{

                                                flag1 = true;

                                                vector<Point> finalv;
                                                finalv.reserve((*it).listRoiCenter.size() + (*itRmLE).listRoiCenter.size());
                                                finalv.insert(finalv.end(),  (*it).listRoiCenter.begin(), (*it).listRoiCenter.end());
                                                finalv.insert(finalv.end(),  (*itRmLE).listRoiCenter.begin(), (*itRmLE).listRoiCenter.end());

                                                vector<vector<PixelEvent> > finalv2;
                                                finalv2.reserve((*it).listRoiPix.size() + (*itRmLE).listRoiPix.size());
                                                finalv2.insert(finalv2.end(),  (*it).listRoiPix.begin(), (*it).listRoiPix.end());
                                                finalv2.insert(finalv2.end(),  (*itRmLE).listRoiPix.begin(), (*itRmLE).listRoiPix.end());


                                                //Update eventMap
                                                vector<Point>::iterator it3;
                                                Mat roi1(roiSize[1], roiSize[0], CV_8UC3, mainColor);
                                                for(it3 = (*itRmLE).listRoiCenter.begin(); it3!=(*itRmLE).listRoiCenter.end(); ++it3){

                                                    roi1.copyTo(eventMap(Rect( (*it3).x  -roiSize[0]/2, (*it3).y -roiSize[1]/2,roiSize[0],roiSize[1])));

                                                }

                                                (*it).listRoiCenter = finalv;

                                                (*it).listRoiPix = finalv2;

                                                (*it).setMap((*it).getMap() + (*itRmLE).getMap()) ;

                                                (*it).listColor.push_back((*itRmLE).getColor());

                                                (*it).computeCenterOfMass(false);

                                                itRmLE = listLE.erase(itRmLE);
                                                it2 = listColorInRoi.erase(it2);

                                            }

                                        }else{

                                            ++it2;

                                        }
                                    }

                                    if(!flag1)
                                        ++itRmLE;

                                    flag1 = false;

                                }
                            }
                       // }
                    }
                }
            }
        }
}

void DetByLists::buildListSubdivisionOriginPoints(vector<Point> &listSubdivPosition, int nbSubdivOnAxis, int imgH, int imgW){


    int subW = imgW/nbSubdivOnAxis;
    int subH = imgH/nbSubdivOnAxis;

    Point first = Point((nbSubdivOnAxis/2 - 1) * subW, (nbSubdivOnAxis/2)*subH);
    Point last = Point(imgW - subW, imgH - subH);

    listSubdivPosition.push_back(first);

    int x = first.x, y = first.y;
    int nbdep = 0,
        nbdepLimit = 1,
        dep = 1; // 1 up
                 // 2 right
                 // 3 down
                 // 4 left

    for(int i = 1; i < nbSubdivOnAxis * nbSubdivOnAxis; i++){

        if(dep == 1){

            y = y - subH;
            listSubdivPosition.push_back(Point(x,y));
            nbdep ++;
            if(nbdep == nbdepLimit){
                nbdep = 0;
                dep ++;
            }

        }else if(dep == 2){

            x = x + subW;
            listSubdivPosition.push_back(Point(x,y));
            nbdep ++;
            if(nbdep == nbdepLimit){
                nbdep = 0;
                nbdepLimit++;
                dep ++;
            }

        }else if(dep == 3){

            y = y + subH;
            listSubdivPosition.push_back(Point(x,y));
            nbdep ++;
            if(nbdep == nbdepLimit){
                nbdep = 0;
                dep ++;
            }

        }else if(dep == 4){

            x = x - subW;
            listSubdivPosition.push_back(Point(x,y));
            nbdep ++;
            if(nbdep == nbdepLimit){
                nbdep = 0;
                nbdepLimit++;
                dep = 1;
            }

        }

    }
}

bool DetByLists::detectionMethodByListManagement(   Frame                   f,
                                                    vector<string>          date,
                                                    Mat                     currentFrame,
                                                    Mat                     previousFrame,
                                                    Mat                     mean,
                                                    int const               *roiSize,
                                                    vector <GlobalEvent>    &listGlobalEvents,
                                                    Mat                     mask,
                                                    boost::mutex            &m_listRecEvent,
                                                    boost::mutex            &mutexQueue,
                                                    vector<RecEvent>        &listRecEvent,
                                                    Fifo<Frame>             &framesQueue,
                                                    string                  recPath,
                                                    string                  stationName,
                                                    int                     &nbDet,
                                                    int                     geMaxDuration,
                                                    int                     geMaxElement,
                                                    int                     geAfterTime,
                                                    int                     pixelFormat,
                                                    vector<Point>           &lastDet,
                                                    Mat                     maskNeighborhood,
                                                    VideoWriter             &videoDebug,
                                                    bool                    debug,
                                                    vector<Point>           listSubdivPosition,

                                                    bool                    downsample,
                                                    Mat &prevthresh           ){

    src::severity_logger< severity_level > log;

    bool rec = false;

    bool saveDiff       = false;
    bool saveThresh     = false;
    bool saveThresh2    = false;
    bool saveEventmap   = false;

    // Height of the frame.
    int imgH = currentFrame.rows;

    // Width of the frame.
    int imgW = currentFrame.cols;

    // Iterator on list of global event.
    vector<GlobalEvent>::iterator itGE;

    // Iterator on list of local event.
    vector<LocalEvent>::iterator itLE;

    // List of localEvent objects.
    vector <LocalEvent> listLocalEvents;

    // Iterator on list of sub-regions.
    vector<Point>::iterator itR;

    // Current frame with a mask.
    Mat currImg;
    currentFrame.copyTo(currImg, mask);

    // Previous frame with a mask.
    Mat prevImg;
    previousFrame.copyTo(prevImg, mask);

    /// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    /// %%%%%%%%%%%%%%%%%%%%%%%%%%% STEP 1 : FILETRING / THRESHOLDING %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    /// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    /*
        -> Frame difference
        -> Compute threshold using standard deviation
        -> Get a binary threshold map
    */

    //cout << "### Starting step 1. ###" << endl;

    // Execution time.
    double tStep1 = (double)getTickCount();

    //cout << "> Downsample : " << downsample << endl;

    double tdownsample = (double)getTickCount();

    // According to DET_DOWNSAMPLE_ENABLED's parameter in configuration file.
    if(downsample){

        imgH /= 2;
        imgW /= 2;
        double tdowncurr = (double)getTickCount();
        pyrDown( currImg, currImg, Size( currImg.cols/2, currImg.rows/2 ) );
        tdowncurr = (((double)getTickCount() - tdowncurr)/getTickFrequency())*1000;
        //cout << "> tdowncurr Time : " << tdowncurr << endl;

        double tdownprev = (double)getTickCount();
        pyrDown( prevImg, prevImg, Size( prevImg.cols/2, prevImg.rows/2 ) );
        tdownprev = (((double)getTickCount() - tdownprev)/getTickFrequency())*1000;
        //cout << "> tdownprev Time : " << tdownprev << endl;


        double tdownmask = (double)getTickCount();
        pyrDown( mask, mask, Size( mask.cols/2, mask.rows/2 ) );
        tdownmask = (((double)getTickCount() - tdownmask)/getTickFrequency())*1000;
        //cout << "> tdownmask Time : " << tdownmask << endl;


    }

    tdownsample = (((double)getTickCount() - tdownsample)/getTickFrequency())*1000;
    //cout << "> Downsample Time : " << tdownsample << endl;

    //cout << "> Compute difference." << endl;

    double tdiff = (double)getTickCount();

    // Difference between current and previous frame.
    Mat diff;
    absdiff(currImg, prevImg, diff);

    if(pixelFormat == MONO_12){

        Conversion::convertTo8UC1(diff).copyTo(diff);
        Conversion::convertTo8UC1(maskNeighborhood).copyTo(maskNeighborhood);

    }

    tdiff = (((double)getTickCount() - tdiff)/getTickFrequency())*1000;
    cout << "> Difference Time : " << tdiff << endl;

    if(saveDiff)
        SaveImg::saveBMP(diff, "/home/fripon/debug/diff/diff_"+Conversion::intToString(f.getNumFrame()));

    //cout << "> Thresholding" << endl;

    double tthresh = (double)getTickCount();

    // Threshold map.
    Mat mapThreshold = Mat(imgH,imgW, CV_8UC1,Scalar(0));

    // Thresholding diff.
    threshold(diff, mapThreshold, defineThreshold(diff, mask), 255, THRESH_BINARY);

    tthresh = (((double)getTickCount() - tthresh)/getTickFrequency())*1000;
    cout << "> Threshold Time : " << tthresh << endl;

    if(saveThresh)
        SaveImg::saveBMP(mapThreshold, "/home/fripon/debug/thresh/thresh_"+Conversion::intToString(f.getNumFrame()));

    double tthresh2 = (double)getTickCount();

    // Remove single white pixel.
    unsigned char * ptrT;

    Mat black(3,3,CV_8UC1,Scalar(0));

    for(int i = 0; i < imgH; i++){

        ptrT = mapThreshold.ptr<unsigned char>(i);

        for(int j = 0; j < imgW; j++){

            if(ptrT[j] > 0){

                Mat t = mapThreshold(Rect( j - 1, i -1,3,3));

                Mat res = t & maskNeighborhood;

                int n = countNonZero(res);

                if(n == 0){

                    if(j-1 > 0 && j+1 < imgW && i-1 > 0 && i +1 < imgH){

                        black.copyTo(mapThreshold(Rect(j-1, i-1, 3, 3)));

                    }
                }
            }
        }
    }

    tthresh2 = (((double)getTickCount() - tthresh2)/getTickFrequency())*1000;
    //cout << "> Eliminate single white pixels Time : " << tthresh2 << endl;

    Mat threshANDprev;

    if(prevthresh.data){

        threshANDprev = mapThreshold & prevthresh;

        //SaveImg::saveBMP(threshANDprev, "/home/fripon/debug/resET_"+Conversion::intToString(f.getNumFrame()));

    }else{

        //cout << "pas encore de data dans prevthresh" << endl;
    }

    mapThreshold.copyTo(prevthresh);

    tStep1 = (((double)getTickCount() - tStep1)/getTickFrequency())*1000;
    cout << "> Time 1) : " << tStep1 << endl << endl;

    if(saveThresh2)
        SaveImg::saveBMP(mapThreshold, "/home/fripon/debug/thresh2/thresh_"+Conversion::intToString(f.getNumFrame()));

    /// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    /// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% STEP 2 : FIND LOCAL EVENT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    /// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    /*
        -> The mapThreshold is subidivided in 64 regions. We begin to analyze the central region and we progressively turn around.
        -> For each white pixel, a region of interest of 10x10 is defined around. This ROI is linked to an existing LE or become a new LE.
    */

    cout << "### Starting step 2. ###" <<endl;

    // Execution time.
    double tStep2 = (double)getTickCount();

    // Event map for the current frame.
    Mat eventMap = Mat(imgH,imgW, CV_8UC3,Scalar(0,0,0));

    //Color used in eventMap to identify localEvents
    Scalar groupColor;
    groupColor.val[0] = 100;    // B
    groupColor.val[1] = 0;      // G
    groupColor.val[2] = 0;      // R

    if(threshANDprev.data){

        for(itR = listSubdivPosition.begin(); itR != listSubdivPosition.end(); ++itR){

             Mat subdivision = threshANDprev(Rect((*itR).x, (*itR).y, imgW/8, imgH/8));

             searchROI( subdivision, threshANDprev, maskNeighborhood, eventMap, groupColor, listLocalEvents, imgH, imgW, roiSize, imgW/8, imgH/8, (*itR),pixelFormat);

        }
    }

    cout << "> LE number : " << listLocalEvents.size() << endl;

    tStep2 = (((double)getTickCount() - tStep2)/getTickFrequency())*1000;
    cout << "> Time 2) : " << tStep2 << endl << endl;

    if(saveEventmap)
        SaveImg::saveBMP(eventMap, "/home/fripon/debug/evMap/event"+Conversion::intToString(f.getNumFrame()));

    /// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    /// %%%%%%%%%%%%%%%%%%%%%%%%%% STEP 3 : ATTACH LE TO GE OR CREATE NEW ONE %%%%%%%%%%%%%%%%%%%%%%%%%
    /// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    /*
        -> Loop list of local events.
        -> For each local events, loop global event list.
        -> If the LE match to a GE, the GE element in GElist is saved.
        -> If another GE match the same LE, we keep the GE wichi is former.
        -> Once we know to which GE the LE can be matched, we add it or chech the direction before to add it.
           The direction is checked each n frames for a GE.
        -> If a LE can't be added to any GE, we create a new GE in GElist if the limit of GE instance has not been reached.
        -> Add the end, the LE is removed from LE list.
        .
    */

    cout << "### Starting step 3. ###" <<endl;

    int nbLink_dir1     = 0;    // Link realized because checking direction is available but not done
    int nbLink_dir2     = 0;    // Link realized because checking direction is available and done
    int nbLink_noDir    = 0;    // Link realized because checking direction is not available
    int nbNewGe         = 0;    // Number of local event transformed in new global event

    double tStep3 = (double)getTickCount();

    itLE = listLocalEvents.begin();

    while(itLE != listLocalEvents.end()){

        bool LELinked = false;
        bool firstGELinked = false;
        vector<GlobalEvent>::iterator itLink;

        //Loop globalEvent's list
        for(itGE = listGlobalEvents.begin(); itGE!=listGlobalEvents.end(); ++itGE){

            Mat res = (*itLE).getMap() & (*itGE).getMapEvent();

            int nbPixNonZero = countNonZero(res);


            if( nbPixNonZero > 0 ){

                LELinked = true;

                if(!firstGELinked){



                    firstGELinked = true;

                    itLink = itGE;

                }else{

                    if((*itGE).getAge() > (*itLink).getAge()){


                        itLink = itGE;

                    }
                }

              //   itLink = itGE;
            }
        }

        // ADD LE to the correct GE
        if(LELinked){

            // Flag utilisé pour setter la propriété de nombre de frame sans qu'un GE ait été attaché à un LE
            (*itLink).setLELinked(true);

            if((*itLink).addLE((*itLE))){

                (*itLE).belongGlobalEv = true;

            }else{

                (*itLink).setLELinked(false);
                (*itLE).notTakeAccount = true;

            }

        }else{

            cout << "nb GE"<<endl;

            if((listGlobalEvents.size()<  geMaxElement)){

                BOOST_LOG_SEV(log,notification) << "Create new globalEvent";

                cout << "CREATE NEW GLOBAL EVENT" << endl;

                GlobalEvent newGlobalEvent(date, imgH, imgW, downsample);

                newGlobalEvent.addLE((*itLE));

                if(newGlobalEvent.getEvPrevBuffer().size() == 0){

                    boost::mutex::scoped_lock lock(mutexQueue);

                    for(int i = framesQueue.getSizeQueue() - 1 ; i>=0; i--){

                        newGlobalEvent.setEvPrevBuffer(framesQueue.getFifoElementAt(i).getImg());

                    }

                    lock.unlock();

                }

                //Add the new globalEvent to the globalEvent's list
                listGlobalEvents.push_back(newGlobalEvent);

                nbNewGe++;

            }

        }
 cout << "erase"<<endl;

        itLE = listLocalEvents.erase(itLE);

    }

    cout << "geMap"<<endl;

    Mat geMap = Mat(imgH,imgW, CV_8UC1,Scalar(0));

    cout << "loop ge list"<<endl;

    //Loop globalEvent's list
    for(itGE=listGlobalEvents.begin(); itGE!=listGlobalEvents.end(); ++itGE){

        //Mat res = (*itGE).getMapEvent() + geMap;

        (*itGE).getMapEvent().copyTo(geMap);


        (*itGE).setAge((*itGE).getAge() + 1);


        if(!(*itGE).getLELinked()){

            (*itGE).setAgeLastElem((*itGE).getAgeLastElem()+1);

        }

        (*itGE).setLELinked(false);

      //  if((*itGE).getEvBuffer().size() != 0){

            (*itGE).setEvBuffer(currentFrame);

        //}

    }

    tStep3 = (((double)getTickCount() - tStep3)/getTickFrequency())*1000;
    cout << "> Time 3) : " << tStep3 << endl;

    /// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    /// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%% STEP 4 : MANAGE LIST GLOBAL EVENT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    /// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    /*
        -> Loop GE list
        -> If a GE has not been updated for 10 frames and that the size of GE is enough, the GE is saved
        -> Else it is removed
    */

    cout << "### Starting step 4. ###" <<endl;

    double  tStep4      = (double)getTickCount();

    int     nbRmGE      = 0;
    int     nbSaveGE    = 0;
    int     trash_cpt   = 0;

    cout << "#GE#       Age          AgeLast" << endl;

    if( listGlobalEvents.size() != 0 ){

        itGE = listGlobalEvents.begin();

        while ( itGE != listGlobalEvents.end() ){

            cout << "#GE#      "<<(*itGE).getAge()<<"          "<<(*itGE).getAgeLastElem()<< endl;

            //No LE added to the current GE for more than 4 frames
            if( (*itGE).getAgeLastElem() > (geAfterTime + 5)|| (*itGE).getPosFailed() > 2 /*|| ((*itGE).getAge() > 500 && (*itGE).getAgeLastElem() > 10)*/ ){

                //Check if the current GE has the minimum required number of LE to considerate it as an event to record
                if( (*itGE).getPosSuccess() >= 2 && (*itGE).getPosFailed() <= 2 ){

                    struct evPathRes r;

                    r = buildRecEventLocation( (*itGE), recPath, stationName );

                    if(r.success){

                       /* SaveImg::saveBMP((*itGE).getDirMap(),"/home/fripon/data2/dirMask_" + Conversion::intToString(f.getNumFrame()) );
                        SaveImg::saveBMP((*itGE).getMapEvent(),"/home/fripon/data2/mapEvent_" + Conversion::intToString(f.getNumFrame()) );*/

                        buildRecEvent((*itGE), r.path, listRecEvent, m_listRecEvent);
                        nbDet++;
                        nbSaveGE ++;
                        rec = true;


                    }

                    itGE = listGlobalEvents.erase(itGE);
 //nbRmGE ++;
                    break;

                }else{

                    //SaveImg::saveBMP((*itGE).getMapEvent(),"/home/fripon/data2/trash/trashGE_" + Conversion::intToString(f.getNumFrame()) + "_" + Conversion::intToString(trash_cpt));

                    trash_cpt++;

                    itGE = listGlobalEvents.erase(itGE);
                    nbRmGE ++;
                }

            }else if((*itGE).getAge() > 400) {

                itGE = listGlobalEvents.erase(itGE);
                nbRmGE ++;

            }else{

                ++itGE;
            }

          /* if(f.getFrameRemaining()< 10 && f.getFrameRemaining() != 0){

                if( (*itGE).getPosFailed() <= 2 && (*itGE).getPosSuccess() >= 2){

                    struct evPathRes r;

                    r = buildRecEventLocation( (*itGE), recPath, stationName );

                    if(r.success){

                        buildRecEvent((*itGE), r.path, listRecEvent, m_listRecEvent);
                        nbDet++;
                        nbSaveGE ++;
                        rec = true;
                        itGE = listGlobalEvents.erase(itGE);
                        break;

                    }
                }
            }*/
        }
    }

    tStep4 = (((double)getTickCount() - tStep4)/getTickFrequency())*1000;

    cout << "Time step4 : " << tStep4 << endl;

    /// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INFOS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    BOOST_LOG_SEV(log,notification) << "GE detected          : " << nbDet;
    BOOST_LOG_SEV(log,notification) << "GE send to recThread : " << nbSaveGE;
    BOOST_LOG_SEV(log,notification) << "GE deleted           : " << nbRmGE + nbSaveGE;

    BOOST_LOG_SEV(log,notification) << "GE in RAM            : " << listGlobalEvents.size();
    cout << "GE in RAM            : " << listGlobalEvents.size()<<endl;
    BOOST_LOG_SEV(log,notification) << ">> Manage GE list    : " << tStep4<< " ms";

    if(debug){

        Mat VIDEO_finalFrame;
        Mat VIDEO_originalFrame;
        Mat VIDEO_diffFrame;
        Mat VIDEO_threshMapFrame;
        Mat VIDEO_eventMapFrame;
        Mat VIDEO_eventFrame;
        Mat VIDEO_geMapFrame;

       // if(f.getNumFrame() < 200){

         /*   int totalDigit = 4;

            int cpt = 0;

            int number = f.getNumFrame();

            int nbZeroToAdd = 0;

            string ch = "";

            if(number<10){

                nbZeroToAdd = totalDigit - 1;

                for(int i = 0; i < nbZeroToAdd; i++){

                    ch += "0";

                }

                SaveImg::saveBMP(currentFrame,"/home/fripon/data2/currentFrame/currentFrame" + ch + Conversion::intToString(f.getNumFrame()));
                SaveImg::saveBMP(copyMapThreshold,"/home/fripon/data2/mapThreshold/mapThreshold_" + ch + Conversion::intToString(f.getNumFrame()));
                SaveImg::saveBMP(eventMap,"/home/fripon/data2/eventMap/evMap_" + ch + Conversion::intToString(f.getNumFrame()));
                SaveImg::saveBMP(geMap,"/home/fripon/data2/ge/geMap_" + Conversion::intToString(f.getNumFrame()));

            }else{

                while(number > 0){

                    number/=10;
                    cpt ++;

                }

                nbZeroToAdd = totalDigit - cpt;

                for(int i = 0; i < nbZeroToAdd; i++){

                    ch += "0";

                }
                SaveImg::saveBMP(currentFrame,"/home/fripon/data2/currentFrame/currentFrame" + ch + Conversion::intToString(f.getNumFrame()));
                SaveImg::saveBMP(copyMapThreshold,"/home/fripon/data2/mapThreshold/mapThreshold_" + ch + Conversion::intToString(f.getNumFrame()));
                SaveImg::saveBMP(eventMap,"/home/fripon/data2/eventMap/evMap_" + ch + Conversion::intToString(f.getNumFrame()));
                SaveImg::saveBMP(geMap,"/home/fripon/data2/ge/geMap_" + Conversion::intToString(f.getNumFrame()));

            }*/
      //  }
/*
        VIDEO_finalFrame        = Mat(960,1280, CV_8UC3,Scalar(255,255,255));

        VIDEO_originalFrame     = Mat(470,630, CV_8UC3,Scalar(0,0,0));
        VIDEO_threshMapFrame    = Mat(470,630, CV_8UC3,Scalar(0,0,0));
        VIDEO_eventMapFrame     = Mat(470,630, CV_8UC3,Scalar(0,0,0));
        VIDEO_eventFrame        = Mat(470,630, CV_8UC3,Scalar(0,0,0));

        cvtColor(copyCurrWithoutMask, copyCurrWithoutMask, CV_GRAY2BGR);
        resize(copyCurrWithoutMask, VIDEO_originalFrame, Size(630,470), 0, 0, INTER_LINEAR );
        cvtColor(copyMapThreshold, copyMapThreshold, CV_GRAY2BGR);
        resize(copyMapThreshold, VIDEO_threshMapFrame, Size(630,470), 0, 0, INTER_LINEAR );
        resize(eventMap, VIDEO_eventMapFrame, Size(630,470), 0, 0, INTER_LINEAR );

        cvtColor(geMap, geMap, CV_GRAY2BGR);

        resize(geMap, VIDEO_eventFrame, Size(630,470), 0, 0, INTER_LINEAR );

        copyMakeBorder(VIDEO_originalFrame, VIDEO_originalFrame, 5, 5, 5, 5, BORDER_CONSTANT, Scalar(255,255,255) );
        copyMakeBorder(VIDEO_threshMapFrame, VIDEO_threshMapFrame, 5, 5, 5, 5, BORDER_CONSTANT, Scalar(255,255,255) );
        copyMakeBorder(VIDEO_eventMapFrame, VIDEO_eventMapFrame, 5, 5, 5, 5, BORDER_CONSTANT, Scalar(255,255,255) );cout << "here"<< endl;
        copyMakeBorder(VIDEO_eventFrame, VIDEO_eventFrame, 5, 5, 5, 5, BORDER_CONSTANT, Scalar(255,255,255) );

        putText(VIDEO_originalFrame, "Original", cvPoint(300,450),FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,0,255), 1, CV_AA);
        putText(VIDEO_threshMapFrame, "Filtering", cvPoint(300,450),FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,0,255), 1, CV_AA);
        putText(VIDEO_eventMapFrame, "Local Event Map", cvPoint(300,450),FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,0,255), 1, CV_AA);
        putText(VIDEO_eventFrame, "Global Event Map", cvPoint(300,450),FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,0,255), 1, CV_AA);

        VIDEO_originalFrame.copyTo(VIDEO_finalFrame(Rect(0, 0, 640, 480)));
        VIDEO_threshMapFrame.copyTo(VIDEO_finalFrame(Rect(640, 0, 640, 480)));
        VIDEO_eventMapFrame.copyTo(VIDEO_finalFrame(Rect(0, 480, 640, 480)));
        VIDEO_eventFrame.copyTo(VIDEO_finalFrame(Rect(640, 480, 640, 480)));

        string printFrameNum = Conversion::intToString(f.getNumFrame());
        const char * c_printFrameNum;
        c_printFrameNum = printFrameNum.c_str();
        putText(VIDEO_finalFrame, c_printFrameNum, cvPoint(30,50),FONT_HERSHEY_COMPLEX_SMALL, 2, cvScalar(0,255,0), 2, CV_AA);

        if(videoDebug.isOpened()){

            videoDebug<<VIDEO_finalFrame;

        }*/

    }

    return rec;

}
