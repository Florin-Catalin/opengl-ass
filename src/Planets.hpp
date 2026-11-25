//
//  Planets.hpp
//  Where Venus, Mars, Jupiter and Saturn appear from Earth, and how bright they are.
//
//  Model: circular, coplanar heliocentric orbits with real radii, periods and J2000 mean
//  longitudes. That is enough to reproduce what you see with the naked eye: Venus never
//  more than ~47 degrees from the Sun (evening/morning star), Mars brightening near
//  opposition, retrograde loops, and slow drift against the stars from night to night.
//

#ifndef Planets_hpp
#define Planets_hpp

#include "glm/glm.hpp"
#include <cmath>

namespace gps {

    const int PLANET_COUNT = 4;

    struct PlanetInfo
    {
        const char* name;
        double orbitRadiusAU;
        double periodDays;
        double longitudeJ2000Deg;
        float eclipticLatitudeDeg; // small fixed offset from the ecliptic (orbit inclination)
        glm::vec3 color;
    };

    // Venus, Mars, Jupiter, Saturn (index order is used by the magnitude formulas below).
    static const PlanetInfo PLANETS[PLANET_COUNT] = {
        { "Venus",   0.7233,   224.701, 181.979,  2.0f, glm::vec3(1.00f, 0.98f, 0.92f) },
        { "Mars",    1.5237,   686.980, 355.433, -1.5f, glm::vec3(1.00f, 0.55f, 0.35f) },
        { "Jupiter", 5.2026,  4332.59,   34.351,  0.8f, glm::vec3(1.00f, 0.95f, 0.85f) },
        { "Saturn",  9.5549, 10759.22,   50.077,  1.8f, glm::vec3(1.00f, 0.90f, 0.65f) },
    };

    static const double EARTH_PERIOD_DAYS = 365.256;
    static const double EARTH_LONGITUDE_J2000_DEG = 100.464;

    struct PlanetSighting
    {
        float elongationDeg; // angle from the Sun along the ecliptic, positive = east (evening sky)
        float latitudeDeg;
        float magnitude;     // apparent visual magnitude (lower is brighter)
    };

    inline glm::dvec2 heliocentric(double radius, double period, double longitudeJ2000, double days)
    {
        double l = glm::radians(longitudeJ2000 + 360.0 * days / period);
        return radius * glm::dvec2(cos(l), sin(l));
    }

    // Apparent magnitude from distances (AU) and phase angle i (degrees), standard formulas.
    inline float planetMagnitude(int index, double r, double delta, double i)
    {
        double m = 5.0 * log10(r * delta);
        switch (index) {
        case 0: return (float)(m - 4.40 + 0.0009 * i + 0.000239 * i * i - 0.00000065 * i * i * i); // Venus
        case 1: return (float)(m - 1.52 + 0.016 * i);  // Mars
        case 2: return (float)(m - 9.40 + 0.005 * i);  // Jupiter
        default: return (float)(m - 9.60 + 0.044 * i); // Saturn, with average ring brightening
        }
    }

    // days: days since J2000. Fills one sighting per planet; returns the Sun's geocentric
    // ecliptic longitude in degrees (used to drift the stars relative to the Sun over a year).
    inline double computePlanets(double days, PlanetSighting out[PLANET_COUNT])
    {
        glm::dvec2 earth = heliocentric(1.0, EARTH_PERIOD_DAYS, EARTH_LONGITUDE_J2000_DEG, days);
        double sunLongitude = glm::degrees(atan2(-earth.y, -earth.x));

        for (int k = 0; k < PLANET_COUNT; k++) {
            const PlanetInfo& p = PLANETS[k];
            glm::dvec2 helio = heliocentric(p.orbitRadiusAU, p.periodDays, p.longitudeJ2000Deg, days);
            glm::dvec2 geo = helio - earth;
            double longitude = glm::degrees(atan2(geo.y, geo.x));
            double elongation = fmod(longitude - sunLongitude + 540.0, 360.0) - 180.0;

            double r = p.orbitRadiusAU;
            double delta = glm::length(geo);
            double cosPhase = (r * r + delta * delta - 1.0) / (2.0 * r * delta);
            double phase = glm::degrees(acos(glm::clamp(cosPhase, -1.0, 1.0)));

            out[k].elongationDeg = (float)elongation;
            out[k].latitudeDeg = p.eclipticLatitudeDeg;
            out[k].magnitude = planetMagnitude(k, r, delta, phase);
        }
        return sunLongitude;
    }
}

#endif /* Planets_hpp */
