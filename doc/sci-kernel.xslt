<?xml version="1.0"?>
<xsl:stylesheet xsl:version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">


<xsl:template match="/kernel-doc">
    <sect2><title>SCI Kernel functions</title>
      <xsl:apply-templates mode="list"/>
    </sect2>
</xsl:template>

<xsl:template match="function" mode="list">
    <sect3>
      <title>
	<xsl:value-of select="normalize-space(text())"/>
	<xsl:text>(</xsl:text>
	<xsl:apply-templates select="param" mode="param-list"/>
	<xsl:text>)
	</xsl:text>
      </title>
      <funcsynopsis>
	<funcprototype>
	  <funcdef>
	    <xsl:apply-templates select="retval" mode="type"/>
	    <xsl:text> </xsl:text>
	    <xsl:value-of select="normalize-space(text())"/>
	  </funcdef>
	  <xsl:text>
	  </xsl:text>
	  <paramdef>
	    <xsl:apply-templates select="param" mode="param-list"/>
	  </paramdef>
	</funcprototype>
      </funcsynopsis>
      <xsl:text>
      </xsl:text>
      <para>
	<simplelist>
	  <xsl:text>
	  </xsl:text>
	  <xsl:apply-templates select="param" mode="detailed-list"/>
	</simplelist>
      </para>
      <para>
	<xsl:apply-templates select="retval" mode="full"/>
      </para>
      <xsl:text>
      </xsl:text>
      <para>
	<xsl:apply-templates select="description" mode="full"/>
      </para>
    </sect3>
</xsl:template>

<xsl:template match="retval" mode="type">
    <xsl:value-of select="substring-before(string(text()), ':')"/>
</xsl:template>

<xsl:template match="retval" mode="full">
    <type>
      <xsl:text>(</xsl:text>
      <xsl:value-of select="substring-before(string(text()), ':')"/>
      <xsl:text>)</xsl:text>
    </type>
    <xsl:text>:</xsl:text>
    <xsl:value-of select="substring-after(string(self::*), ':')"/>
</xsl:template>

<xsl:template match="param" mode="param-list">
    <xsl:value-of select="substring-before(string(text()), ':')"/>
    <xsl:if test="position() &lt; last()">, </xsl:if>
</xsl:template>

<xsl:template match="param" mode="detailed-list">
    <member>
      <type>
	<xsl:text>(</xsl:text>
	<xsl:value-of select="substring-before(string(text()), ' ')"/>
	<xsl:text>)</xsl:text>
      </type>
      <parameter>
	<xsl:value-of
	  select="substring-before(substring-after(string(text()), ' '), ':')"/>
      </parameter>
      <xsl:text>:</xsl:text>
      <xsl:value-of
	select="substring-after(string(text()), ':')"/>
    </member>
    <xsl:text>
    </xsl:text>
</xsl:template>

<xsl:template match="type" mode="full">
    <type>
      <xsl:apply-templates mode="full"/>
    </type>
</xsl:template>

<xsl:template match="slc" mode="full">
    <literal>
      <xsl:apply-templates mode="full"/>
    </literal>
</xsl:template>

<xsl:template match="literal" mode="full">
    <literal>
      <xsl:apply-templates mode="full"/>
    </literal>
</xsl:template>

</xsl:stylesheet>






