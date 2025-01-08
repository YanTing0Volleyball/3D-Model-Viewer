#version 330 core

// Data from vertex shader.
in vec3 iPosWorld;
in vec3 iNormalWorld;
in vec2 iTexCoord;


// Material properties.
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float Ns;

// Camera Position
uniform vec3 cameraPos;

// Light data.
uniform vec3 ambientLight;

// Directional Light
uniform vec3 dirLightDir;
uniform vec3 dirLightRadiance;

// Point Light
uniform vec3 pointLightPos;
uniform vec3 pointLightIntensity;

// Spot Light
uniform vec3 spotLightPos;
uniform vec3 spotLightIntensity;
uniform vec3 spotLightDir;
uniform float spotLightTotalWidth;
uniform float spotLightFoS;
uniform float cosSpotLightTotalWidth;
uniform float cosSpotLightFos;

// Lighting Mode
uniform int lightingMode;

// Texture Data
uniform sampler2D mapKd;
uniform bool useMapKd;

out vec4 FragColor;


vec3 Diffuse(vec3 Kd, vec3 I, vec3 N, vec3 lightDir)
{
    return Kd * I * max(0, dot(N, lightDir));
}

vec3 Specular(vec3 Ks, vec3 I, vec3 N, vec3 lightDir, vec3 viewDir, float Ns)
{
    //Blinn-Phong
    vec3 vH = normalize(lightDir + viewDir);    

    return Ks * I * pow(max(0, dot(N, vH)), Ns);
}

float getCos(vec3 v1, vec3 v2){
   vec3 n1 = normalize(v1);
   vec3 n2 = normalize(v2);

   return dot(n1, n2);
}

float degreeToCos(float degree){
   return cos(radians(degree));
}

float getDegree(vec3 v1, vec3 v2){
  float cosValue = getCos(v1, v2);
  
  return degrees(acos(cosValue));
}

void main()
{
    vec3 N = normalize(iNormalWorld);
    vec3 worldLightDir;
    vec3 diffuse;
    vec3 specular;
    vec3 worldViewDir = normalize(cameraPos - iPosWorld);
    vec3 texKd = useMapKd ? texture2D(mapKd, iTexCoord).rgb : Kd;

    // For Spot Light & Point Light To Calculate Local Ligth Intensity
    float attenuation;
    vec3 radiance;
    float distSurfaceToLight;

    // Ligth Calculate
    
    //----------------------------------------------------------------
    // Ambient Light
       vec3 ambient = Ka * ambientLight;
    //----------------------------------------------------------------
    

    //----------------------------------------------------------------
    // Directional Light
       worldLightDir = normalize(-dirLightDir);      

       //Diffuse
       //diffuse = Diffuse(Kd, dirLightRadiance, N, worldLightDir);
       diffuse = Diffuse(texKd, dirLightRadiance, N, worldLightDir);

       //Specular
       specular = Specular(Ks, dirLightRadiance, N, worldLightDir, worldViewDir, Ns);
       
       //Directional Light Sum
       vec3 dirLight = diffuse + specular;
    //----------------------------------------------------------------
    // Point Light
       worldLightDir = normalize(pointLightPos - iPosWorld);
       
       distSurfaceToLight = distance(pointLightPos, iPosWorld);
    
       attenuation = 1.0f / (distSurfaceToLight * distSurfaceToLight);
    
       radiance = pointLightIntensity * attenuation;
    
       //Diffuse
       //diffuse = Diffuse(Kd, radiance, N, worldLightDir);
       diffuse = Diffuse(texKd, radiance, N, worldLightDir);


       //Specular
       specular = Specular(Ks, radiance, N, worldLightDir, worldViewDir, Ns);
    
       vec3 pointLight = diffuse + specular;
    //----------------------------------------------------------------
    // Spot Light
       worldLightDir = normalize(spotLightPos - iPosWorld);
       
       
       float angleA = getDegree(-normalize(spotLightDir), worldLightDir);
       
       float cosA = degreeToCos(angleA);
       //float cosT = degreeToCos(spotLightTotalWidth);
       //float cosF = degreeToCos(spotLightFoS);
       
       radiance = spotLightIntensity * clamp((cosA - cosSpotLightTotalWidth) / (cosSpotLightFos - cosSpotLightTotalWidth), 0.0, 1.0);
       //radiance = spotLightIntensity * clamp((cosA - cosT) / (cosF - cosT), 0.0, 1.0);


       distSurfaceToLight = distance(spotLightPos, iPosWorld);
       attenuation = 1.0f / (distSurfaceToLight * distSurfaceToLight);

       radiance *= attenuation;

       //Diffuse
       //diffuse = Diffuse(Kd, radiance, N, worldLightDir);
       diffuse = Diffuse(texKd, radiance, N, worldLightDir);


       //Specular
       specular = Specular(Ks, radiance, N, worldLightDir, worldViewDir, Ns);

       vec3 spotLight = diffuse + specular;

    //----------------------------------------------------------------

   vec3 lightingColor = ambient;   

   // According To Lighting Mode Use Differnt Light
   switch(lightingMode){
   case 0:
     lightingColor += dirLight + pointLight + spotLight;
     break;
   case 1:
     lightingColor += dirLight;
     break;
   case 2:
     lightingColor += pointLight;
     break;
   case 3:
     lightingColor += spotLight;
     break;
   }


    //FragColor = vec4(N, 1.0);
    FragColor = vec4(lightingColor, 1.0);
}
