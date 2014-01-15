#include "boundaryPoint.h"

#include "blockMeshTopology.h"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::boundaryPoint::boundaryPoint
(
    const std::set<std::set<Foam::label> > &triangles,
    const point &initialPoint,
    blockMeshTopology *topo
)
    :
    pointTopo(topo),
    triangles_(triangles),
    trianglesNew_(triangles),
    initialPoint_(initialPoint)
{
}

Foam::boundaryPoint::~boundaryPoint()
{
}

// * * * * * * * * * * * * * * * Private Functions * * * * * * * * * * * * * //

bool Foam::boundaryPoint::isOnTriangle
(
    const label &refP2,
    const label &refP3,
    const point &p,
    point &out,
    const label &ref
) const
{
    // http://math.stackexchange.com/questions/544946/determine-if-projection-of-3d-point-onto-plane-is-within-a-triangle/

    const point &p1(topo_->getBoundaryPointCoord(ref));
    const point &p2(topo_->getBoundaryPointCoord(refP2));
    const point &p3(topo_->getBoundaryPointCoord(refP3));

    const point u(p2 - p1);
    const point v(p3 - p1);
    const point n(u ^ v);
    const point w(p - p1);

    const scalar lambda(fabs((u ^ w) & n)/(n & n));
    const scalar beta(fabs((w ^ v) & n)/(n & n));
    const scalar alpha(1.0 - lambda - beta);

    if
    (
        alpha >= 0 && alpha <= 1 &&
        lambda >= 0 && lambda <= 1 &&
        beta >= 0 && beta <= 1
    )
    { // Point is on triangle

        // Compute the projected point
        out = alpha*p1 + beta*p2 + lambda*p3;
        return true;
    }
    else
    {
        return false;
    }
}

std::set<std::set<Foam::label> > Foam::boundaryPoint::getTrianglesLinked() const
{
    return triangles_;
}

Foam::point &Foam::boundaryPoint::getboundaryPoint()
{
    return initialPoint_;
}

Foam::point Foam::boundaryPoint::projectedBndPoint
(
    const point &guessedPoint,
    const label &ref
)
{
    std::map<scalar,point> minDists(mapNeiborFeaturePts(guessedPoint, ref));

    if (minDists.empty())
    {
        const scalar distCenter(mag(guessedPoint - topo_->getBoundaryPointCoord(ref)));

        // Set of all extremity point
        std::set<label> extremPoint;
        for // all triangle
        (
            std::set<std::set<label> >::iterator triI = trianglesNew_.begin();
            triI != trianglesNew_.end();
            ++triI
        )
        {
            for // all point
            (
                std::set<label>::iterator ptI = triI->begin();
                ptI != triI->end();
                ++ptI
            )
            {
                 extremPoint.insert(*ptI);
            }
        }

        // Store the dist between guessed point and extremity point
        std::map<scalar, label> dist;

        for // all extremity point
        (
            std::set<label>::iterator ptI = extremPoint.begin();
            ptI != extremPoint.end();
            ++ptI
        )
        {
            dist.insert
            (
                std::make_pair<scalar, label>
                (
                    mag(guessedPoint - topo_->getBoundaryPointCoord(*ptI)),
                    *ptI
                )
            );
        }

        if (distCenter < dist.begin()->first)
        { // convex
            return topo_->getBoundaryPointCoord(ref);
        }
        else
        {
            return changeFeatureEdgeLinkedsPoint(dist.begin()->second, guessedPoint);
        }
    }
    else
    {
        return minDists.begin()->second;
    }
}

Foam::point Foam::boundaryPoint::changeFeatureEdgeLinkedsPoint
(
    const Foam::label &newRef,
    const Foam::point &guessedPoint
)
{
    FatalErrorIn("changeFeatureEdgeLinkedsPoint(guessedPoint, ref)")
        << "Accessed from a non feature edge point\n"
        << nl
        << exit(FatalError);

    return point();
}

Foam::point Foam::boundaryPoint::changeBoundaryPointLinkedFaces
(
    const label &newRef,
    const point &guessedPoint
)
{
    // Update the faces linked
    trianglesNew_ = topo_->getPointTopoPtr(newRef)->getTrianglesLinked();

    return getBoundaryPoint(guessedPoint, newRef);
}

Foam::point Foam::boundaryPoint::getInitialPoint(const Foam::label &ref) const
{
    return topo_->getBoundaryPointCoord(ref);
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


Foam::point Foam::boundaryPoint::smoothedPoint
(
    const Foam::point &guessedPoint,
    const label &pointRef
)
{
    return getBoundaryPoint(guessedPoint, pointRef);
}

std::map<Foam::scalar, Foam::point> Foam::boundaryPoint::mapNeiborFeaturePts
(
    const point &guessedPoint,
    const label &pointRef
)
{
    FatalErrorIn("mapNeiborFeaturePts(guessedPoint, ref)")
        << "Not a feature point\n"
        << nl
        << exit(FatalError);

    return std::map<Foam::scalar, Foam::point>();
}

std::map<Foam::scalar, Foam::point> Foam::boundaryPoint::mapBoundaryFeaturePts
(
    const point &guessedPoint,
    const label &pointRef
)
{
    std::map<scalar,point> minDist;

    for
    (
        std::set<std::set<label> >::iterator triI = trianglesNew_.begin();
        triI != trianglesNew_.end();
        ++triI
    )
    {
        point pt;
        if
        (
             isOnTriangle
             (
                 *(*triI).begin(),
                 *(*triI).rbegin(),
                 guessedPoint,
                 pt,
                 pointRef
             )
         )
        {
            minDist.insert
            (
                std::make_pair<scalar,point>(mag(guessedPoint - pt), pt)
            );
        }
    }

    return minDist;
}


Foam::point Foam::boundaryPoint::getFeatureEdgePoint
(
    const Foam::point &guessedPoint,
    const Foam::label &ref
)
{
    FatalErrorIn("getFeatureEdgePoint(guessedPoint, ref)")
        << "Error no feature edge point\n"
        << nl
        << exit(FatalError);

    return point();
}

Foam::point Foam::boundaryPoint::getBoundaryPoint
(
    const point &guessedPoint,
    const label &ref
)
{
    std::map<scalar,point> minDists(mapBoundaryFeaturePts(guessedPoint, ref));

    if (minDists.empty())
    { // No projection found
        const scalar distCenter(mag(guessedPoint - topo_->getBoundaryPointCoord(ref)));

        // Set of all extremity point
        std::set<label> extremPoint;
        for // all triangle
        (
            std::set<std::set<label> >::iterator triI = trianglesNew_.begin();
            triI != trianglesNew_.end();
            ++triI
        )
        {
            for // all point
            (
                std::set<label>::iterator ptI = triI->begin();
                ptI != triI->end();
                ++ptI
            )
            {
                 extremPoint.insert(*ptI);
            }
        }

        // Store the dist between guessed point and extremity point
        std::map<scalar, label> dist;

        for // all extremity point
        (
            std::set<label>::iterator ptI = extremPoint.begin();
            ptI != extremPoint.end();
            ++ptI
        )
        {
            dist.insert
            (
                std::make_pair<scalar, label>
                (
                    mag(guessedPoint - topo_->getBoundaryPointCoord(*ptI)),
                    *ptI
                )
            );
        }

        if (distCenter < dist.begin()->first)
        { // convex, return the closest point
            return topo_->getBoundaryPointCoord(ref);
        }
        else
        { // concave update the conected faces with thoses arround closest point
            return changeBoundaryPointLinkedFaces
            (
                dist.begin()->second,
                guessedPoint
            );
        }
    }
    else
    {
        return minDists.begin()->second;
    }
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


// ************************************************************************* //
