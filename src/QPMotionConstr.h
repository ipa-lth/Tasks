// This file is part of Tasks.
//
// Tasks is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tasks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Tasks.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

// includes
// std
#include <map>

// Eigen
#include <Eigen/Core>

// RBDyn
#include <RBDyn/FD.h>
#include <RBDyn/Jacobian.h>

// Tasks
#include "QPSolver.h"


namespace tasks
{

namespace qp
{

class MotionConstrCommon : public ConstraintFunction<GenInequality, Bound>
{
public:
	MotionConstrCommon();

	void computeTorque(int robotIndex,
										const Eigen::VectorXd& alphaD,
										const Eigen::VectorXd& lambda);
	const Eigen::VectorXd& torque(int robotIndex) const;
	void torque(const std::vector<rbd::MultiBody>& mbs,
		std::vector<rbd::MultiBodyConfig>& mbcs,
		int robotIndex) const;

	// Constraint
	virtual void updateNrVars(const std::vector<rbd::MultiBody>& mbs,
		const SolverData& data);

	void computeMatrix(const std::vector<rbd::MultiBody>& mb,
		const std::vector<rbd::MultiBodyConfig>& mbcs);

	// Description
	virtual std::string nameGenInEq() const;
	virtual std::string descGenInEq(const std::vector<rbd::MultiBody>& mbs, int line);
	virtual std::string nameBound() const;
	virtual std::string descBound(const std::vector<rbd::MultiBody>& mbs, int line);

	// Inequality Constraint
	virtual int maxGenInEq() const;

	virtual const Eigen::MatrixXd& AGenInEq() const;
	virtual const Eigen::VectorXd& LowerGenInEq() const;
	virtual const Eigen::VectorXd& UpperGenInEq() const;

	// Bound Constraint
	virtual int beginVar() const;

	virtual const Eigen::VectorXd& Lower() const;
	virtual const Eigen::VectorXd& Upper() const;

protected:
	struct ContactData
	{
		ContactData() {}
		ContactData(const rbd::MultiBody& mb,
			int bodyId, int lambdaBegin,
			std::vector<Eigen::Vector3d> points,
			const std::vector<FrictionCone>& cones);


		int bodyIndex, lambdaBegin;
		rbd::Jacobian jac;
		std::vector<Eigen::Vector3d> points;
		std::vector<Eigen::Matrix<double, 3, Eigen::Dynamic> > generators;
		// Hold the translated jacobian
		Eigen::MatrixXd jacTrans;
		// Hold the generator in world frame
		// std::vector<Eigen::Matrix<double, 3, Eigen::Dynamic> > generatorsComp;
	};

	struct RobotData
	{
		RobotData() {}
		RobotData(const rbd::MultiBody& mb, int robotIndex,
			int alphaDBegin, int aRow,
			std::vector<ContactData> contacts);

		int robotIndex, alphaDBegin, nrDof, ARow;
		rbd::ForwardDynamics fd;
		std::vector<ContactData> cont;
		Eigen::MatrixXd fullJac;

		Eigen::VectorXd curTorque;
	};

protected:
	std::vector<RobotData> robots_;
	int lambdaBegin_;

	Eigen::MatrixXd A_;
	Eigen::VectorXd AL_, AU_;

	Eigen::VectorXd XL_, XU_;

	std::map<int, int> rIndexToRobot_;
};


struct TorqueBounds
{
	TorqueBounds() {}
	TorqueBounds(std::vector<std::vector<double>> lTorqueBounds,
		std::vector<std::vector<double>> uTorqueBounds);


	std::vector<std::vector<double>> lTorqueBounds, uTorqueBounds;
};


class MotionConstr : public MotionConstrCommon
{
public:
	MotionConstr(const std::vector<rbd::MultiBody>& mbs,
							 std::vector<TorqueBounds> tbs);

	// Constraint
	virtual void update(const std::vector<rbd::MultiBody>& mbs,
		const std::vector<rbd::MultiBodyConfig>& mbcs,
		const SolverData& data);

protected:
	struct TorqueBoundsData
	{
		int alphaDOffset;
		Eigen::VectorXd torqueL, torqueU;
	};

protected:
	std::vector<TorqueBoundsData> torqueBounds_;
};


struct SpringJoint
{
	SpringJoint(){}
	SpringJoint(int rIndex, int jId, double K, double C, double O):
		robotIndex(rIndex),jointId(jId),K(K),C(C),O(O)
	{}

	int robotIndex, jointId;
	double K, C, O;
};


class MotionSpringConstr : public MotionConstr
{
public:
	MotionSpringConstr(const std::vector<rbd::MultiBody>& mbs,
										std::vector<TorqueBounds> tbs,
										const std::vector<SpringJoint>& springs);

	// Constraint
	virtual void update(const std::vector<rbd::MultiBody>& mbs,
		const std::vector<rbd::MultiBodyConfig>& mbc,
		const SolverData& data);

protected:
	struct SpringJointData
	{
		int index;
		int posInDof;
		double K;
		double C;
		double O;
	};

protected:
	std::vector<std::vector<SpringJointData>> springs_;
};


struct PolyTorqueBounds
{
	PolyTorqueBounds() {}
	PolyTorqueBounds(std::vector<std::vector<Eigen::VectorXd>> lPolyTorqueBounds,
		std::vector<std::vector<Eigen::VectorXd>> uPolyTorqueBounds);


	std::vector<std::vector<Eigen::VectorXd>> lPolyTorqueBounds, uPolyTorqueBounds;
};


/**
 * @brief Use polynome in function of q to compute torque limits.
 * BEWARE: Only work with 1 dof/param joint
 */
class MotionPolyConstr : public MotionConstrCommon
{
public:
	MotionPolyConstr(const std::vector<rbd::MultiBody>& mbs,
		const std::vector<PolyTorqueBounds>& ptbs);

	// Constraint
	virtual void update(const std::vector<rbd::MultiBody>& mbs,
		const std::vector<rbd::MultiBodyConfig>& mbcs,
		const SolverData& data);

protected:
	struct PolyTorqueBoundsData
	{
		std::vector<Eigen::VectorXd> torqueL, torqueU;
		std::vector<int> jointIndex;
	};

protected:
	std::vector<PolyTorqueBoundsData> polyTorqueBounds_;
};


} // namespace qp

} // namespace tasks
