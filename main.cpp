/**
 * Dempster-Shafer Library for Evidence-Theory
 * Thilo Michael, Jeffrey Jedele
 * 2012
 * > lab exercise, main file
 */

#include "dempstershafer.hpp"
#include "learningclassificator.hpp"
#include "csvreader.hpp"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

string *hypotheseses;

string hypothesis_to_string_function(void* element);
void print_frame_stats(int frame_no,
				int eye_value,
				int mouth_value,
				int furrow_value,
				double eye_classification,
				double mouth_classification,
				double furrow_classification,
				Evidence& eye_evidence,
				Evidence& mouth_evidence,
				Evidence& furrow_evidence,
				Evidence& combined);

// hypotheseses
string fear("fear");
string surprise("surprise");
string disdain("disdain");
string disgust("disgust");
string anger("anger");

int main(int argc, char** argv) {
	// create the CSV reader
	if(argc < 2) {
		cerr << "Provide .csv file with video data as argument!" << endl;
		return EXIT_FAILURE;
	}
	CSVReader csv(argv[1]);

	// create Dempster-Shafer universe
	DempsterShaferUniverse universe;
	universe.add_hypotheseses(&fear, &surprise, &disdain, &disgust, &anger, NULL);

	// create the feature classificator with online learning
	LearningClassificator classificator(0.05, 3);
	const int EYE_APERTURE = classificator.add_feature(18.0);
	const int MOUTH_OPENING = classificator.add_feature(23.0);
	const int FURROW_COUNT = classificator.add_feature(527.0);

	// create bitset representations of emotions sets to save performance while classifying video frames
	bitset<MAX_HYPOTHESESES> fear_and_surprise = universe.bitset_representation(&fear, &surprise, NULL);
	bitset<MAX_HYPOTHESESES> disdain_and_disgust = universe.bitset_representation(&disdain, &disgust, NULL);
	bitset<MAX_HYPOTHESESES> only_surprise = universe.bitset_representation(&surprise, NULL);
	bitset<MAX_HYPOTHESESES> anger_and_disgust = universe.bitset_representation(&anger, &disgust, NULL);
	bitset<MAX_HYPOTHESESES> only_anger = universe.bitset_representation(&anger, NULL);

	// classify the frames
	for(int i=0; i<csv.number_of_rows(); i++) {
		vector<int> frame = csv.row(i);

		// evidence for eye aperture
		Evidence eye_aperture = universe.add_evidence();
		double eye_aperture_classification = classificator.classify(EYE_APERTURE, frame.at(1));
		eye_aperture_classification *= 0.9; // we don't want 1.0 as mass
		if(eye_aperture_classification >= 0.0) {
			// large eye aperture
			eye_aperture.add_focal_set(eye_aperture_classification, fear_and_surprise);
		} else {
			// small eye aperture
			eye_aperture.add_focal_set(-eye_aperture_classification, disdain_and_disgust);
		}
		eye_aperture.add_omega_set();

		// evidence for mouth opening
		Evidence mouth_opening = universe.add_evidence();
		double mouth_opening_classification = classificator.classify(MOUTH_OPENING, frame.at(2));
		mouth_opening_classification *= 0.9; // we don't want 1.0 as mass
		if(mouth_opening_classification >= 0.0) {
			// large mouth opening
			mouth_opening.add_focal_set(mouth_opening_classification, only_surprise);
		} else {
			// small mouth opening
			mouth_opening.add_focal_set(-mouth_opening_classification, anger_and_disgust);
		}
		mouth_opening.add_omega_set();

		// evidence for furrow count
		Evidence furrow_count = universe.add_evidence();
		double furrow_count_classification = classificator.classify(FURROW_COUNT, frame.at(3));
		furrow_count_classification *= 0.9; // we don't want 1.0 as mass
		if(furrow_count_classification >= 0.0) {
			// many furrows
			furrow_count.add_focal_set(furrow_count_classification, fear_and_surprise);
		} else {
			// few furrows
			furrow_count.add_focal_set(-furrow_count_classification, only_anger);
		}
		furrow_count.add_omega_set();

		// combine the features
		Evidence combined_features = eye_aperture & mouth_opening & furrow_count;

		// find the most plausible emotion
		print_frame_stats(frame.at(0),
						frame.at(1),
						frame.at(2),
						frame.at(3),
						eye_aperture_classification,
						mouth_opening_classification,
						furrow_count_classification,
						eye_aperture,
						mouth_opening,
						furrow_count,
						combined_features);

		// frame could be classified here to remove the ugly print function
		//string* emotion = (string*) combined_features.best_match();
		//cout << "Frame: " << frame.at(0) << " classified as " << *emotion << "." << endl;
	}

	return EXIT_SUCCESS;
}

string hypothesis_to_string_function(void* element) {
	string *s = (string*) element;
	return *s;
}

// very ugly code for debugging + demonstration below here
void print_frame_stats(int frame_no,
				int eye_value,
				int mouth_value,
				int furrow_value,
				double eye_classification,
				double mouth_classification,
				double furrow_classification,
				Evidence& eye_evidence,
				Evidence& mouth_evidence,
				Evidence& furrow_evidence,
				Evidence& combined)
{
	printf("---------------------------------\n");
	printf("### Frame: %03d ###\n", frame_no);
	printf("---------------------------------\n");
	printf("(-1.0: far below average, +1.0 far above average)\n");
	printf("Eye Aperture:  %3d -> %04.2f\n", eye_value, eye_classification);
	printf("Mouth Opening: %3d -> %04.2f\n", mouth_value, mouth_classification);
	printf("Furrow Count:  %3d -> %04.2f\n", furrow_value, furrow_classification);
	printf("---------------------------------\n");
	printf("(#: Belief, -: Plausability, .: nothing)\n");
	
	string bar("");
	int count;
	int belief;
	int add_plaus;
	const int BAR_LENGTH = 50;

	// fear
	count = 0;
	bar = "";
	belief = (int) 100*combined.belief(&fear, NULL);
	add_plaus = (int) 100*combined.plausability(&fear, NULL) - belief;
	for(int i=0; i<belief/(100/BAR_LENGTH); i++, count++) bar.append("#");
	for(int i=0; i<add_plaus/(100/BAR_LENGTH); i++, count++) bar.append("-");
	for(;count<BAR_LENGTH;count++) bar.append(".");
	printf("Fear     | %s \n", bar.c_str());

	// surprise 
	count = 0;
	bar = "";
	belief = (int) 100*combined.belief(&surprise, NULL);
	add_plaus = (int) 100*combined.plausability(&surprise, NULL) - belief;
	for(int i=0; i<belief/(100/BAR_LENGTH); i++, count++) bar.append("#");
	for(int i=0; i<add_plaus/(100/BAR_LENGTH); i++, count++) bar.append("-");
	for(;count<BAR_LENGTH;count++) bar.append(".");
	printf("Surprise | %s \n", bar.c_str());

	// disdain
	count = 0;
	bar = "";
	belief = (int) 100*combined.belief(&disdain, NULL);
	add_plaus = (int) 100*combined.plausability(&disdain, NULL) - belief;
	for(int i=0; i<belief/(100/BAR_LENGTH); i++, count++) bar.append("#");
	for(int i=0; i<add_plaus/(100/BAR_LENGTH); i++, count++) bar.append("-");
	for(;count<BAR_LENGTH;count++) bar.append(".");
	printf("Disdain  | %s \n", bar.c_str());

	// disgust
	count = 0;
	bar = "";
	belief = (int) 100*combined.belief(&disgust, NULL);
	add_plaus = (int) 100*combined.plausability(&disgust, NULL) - belief;
	for(int i=0; i<belief/(100/BAR_LENGTH); i++, count++) bar.append("#");
	for(int i=0; i<add_plaus/(100/BAR_LENGTH); i++, count++) bar.append("-");
	for(;count<BAR_LENGTH;count++) bar.append(".");
	printf("Disgust  | %s \n", bar.c_str());

	// anger 
	count = 0;
	bar = "";
	belief = (int) 100*combined.belief(&anger, NULL);
	add_plaus = (int) 100*combined.plausability(&anger, NULL) - belief;
	for(int i=0; i<belief/(100/BAR_LENGTH); i++, count++) bar.append("#");
	for(int i=0; i<add_plaus/(100/BAR_LENGTH); i++, count++) bar.append("-");
	for(;count<BAR_LENGTH;count++) bar.append(".");
	printf("Anger    | %s \n", bar.c_str());

	printf("---------------------------------\n");
	cout << "classified as: " << hypothesis_to_string_function(combined.best_match()) << endl;
	printf("---------------------------------\n");

	cout << endl;
}
