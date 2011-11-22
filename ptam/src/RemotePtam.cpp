/*
 * RemotePtam.cpp
 *
 *  Created on: Jul 8, 2010
 *      Author: acmarkus
 *
 *  parts taken from ros_cturtle/stacks/image_pipeline/image_view/src/image_view.cpp
 */


/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2008, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/



#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <cv_bridge/CvBridge.h>
#include <image_transport/image_transport.h>

#include <boost/thread.hpp>
#include <boost/format.hpp>

#include <ptam_srvs/ptam_command.h>
#include <std_msgs/String.h>

#ifdef HAVE_GTK
#include <gtk/gtk.h>

// Platform-specific workaround for #3026: image_view doesn't close when
// closing image window. On platforms using GTK+ we connect this to the
// window's "destroy" event so that image_view exits.
static void destroy(GtkWidget *widget, gpointer data)
{
	ros::shutdown();
}
#endif

class ImageView
{
private:
	image_transport::Subscriber *sub_;

	sensor_msgs::ImageConstPtr last_msg_;
	sensor_msgs::CvBridge img_bridge_;
	boost::mutex image_mutex_;

	std::string window_name_;
	boost::format filename_format_;
	int count_;
	std::string transport_;
	std::string topic_;

public:
	ImageView(const ros::NodeHandle& nh, const std::string& _transport)
	: filename_format_(""), count_(0)
	{
		topic_ = "vslam/preview";
		ros::NodeHandle local_nh("~");
		local_nh.param("window_name", window_name_, topic_);

		transport_ = _transport;

		bool autosize;
		local_nh.param("autosize", autosize, false);

		const char* name = window_name_.c_str();
		cvNamedWindow(name, autosize ? CV_WINDOW_AUTOSIZE : 0);
#ifdef HAVE_GTK
		GtkWidget *widget = GTK_WIDGET( cvGetWindowHandle(name) );
		g_signal_connect(widget, "destroy", G_CALLBACK(destroy), NULL);
#endif
		cvStartWindowThread();


		sub_ = NULL;
		subscribe(nh);
	}

	~ImageView()
	{
		unsubscribe();
		cvDestroyWindow(window_name_.c_str());
	}

	void image_cb(const sensor_msgs::ImageConstPtr& msg)
	{
		boost::lock_guard<boost::mutex> guard(image_mutex_);

		// Hang on to message pointer for sake of mouse_cb
		last_msg_ = msg;

		// May want to view raw bayer data
		// NB: This is hacky, but should be OK since we have only one image CB.
		if (msg->encoding.find("bayer") != std::string::npos)
			boost::const_pointer_cast<sensor_msgs::Image>(msg)->encoding = "mono8";

		if (img_bridge_.fromImage(*msg, "bgr8"))
			cvShowImage(window_name_.c_str(), img_bridge_.toIpl());
		else
			ROS_ERROR("Unable to convert %s image to bgr8", msg->encoding.c_str());
	}
	void subscribe(const ros::NodeHandle& nh){
		if(sub_ == NULL){
			image_transport::ImageTransport it(nh);
			sub_ = new image_transport::Subscriber(it.subscribe(topic_, 1, &ImageView::image_cb, this, transport_));
		}
	}

	void unsubscribe(){
		if(sub_ != NULL){
			delete sub_;
			sub_ = NULL;
		}
	}
};

int main(int argc, char **argv)
{
	ros::init(argc, argv, "vslam_remote", ros::init_options::AnonymousName);
	ros::NodeHandle n;
//	if (n.resolveName("image") == "/image") {
//		ROS_WARN("image_view: image has not been remapped! Typical command-line usage:\n"
//				"\t$ ./image_view image:=<image topic> [transport]");
//	}

	ImageView view(n, (argc > 1) ? argv[1] : "raw");

	char key = 0;

	view.subscribe(n);
	bool subscribed = true;

//	ros::ServiceClient commandClient = n.serviceClient<sfly_srvs::ptam_command>("vslam/command");
//	sfly_srvs::ptam_commandRequest req;
//	sfly_srvs::ptam_commandResponse res;

	ros::Publisher key_pub = n.advertise<std_msgs::String>("vslam/key_pressed", 10);
	std_msgs::StringPtr msg(new std_msgs::String);

	while(ros::ok()){
		key = cvWaitKey(10);

		if(key == ' '){
			std::cout<<"Sending \"Space\" to ptam"<<std::endl;
//			req.command = "Space";
//			std::cout<< (commandClient.call(req, res) ? "OK" : "Failed")<<std::endl;
			msg->data = "Space";
			key_pub.publish(msg);
		}
		else if(key == 'r'){
			std::cout<<"Sending \"r\" to ptam"<<std::endl;
//			req.command = "r";
//			std::cout<< (commandClient.call(req, res) ? "OK" : "Failed")<<std::endl;
                        msg->data = "r";
                        key_pub.publish(msg);
		}
		else if(key == 'a'){
			std::cout<<"Sending \"a\" to ptam"<<std::endl;
//			req.command = "a";
//			std::cout<< (commandClient.call(req, res) ? "OK" : "Failed")<<std::endl;
                        msg->data = "a";
                        key_pub.publish(msg);
		}
		else if(key == 'q'){
			std::cout<<"Sending \"q\" to ptam"<<std::endl;
//			req.command = "q";
//			std::cout<< (commandClient.call(req, res) ? "OK" : "Failed")<<std::endl;
                        msg->data = "q";
                        key_pub.publish(msg);
		}
		else if(key == 's'){
			if(subscribed){
				view.unsubscribe();
				subscribed = false;
				std::cout<<"unsubscribed"<<std::endl;
			}
			else{
				view.subscribe(n);
				subscribed = true;
				std::cout<<"subscribed"<<std::endl;
			}
		}

		ros::spinOnce();
	}
	return 0;
}
